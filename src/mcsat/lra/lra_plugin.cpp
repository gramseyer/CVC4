#include "mcsat/lra/lra_plugin.h"

using namespace CVC4;
using namespace mcsat;
using namespace lra;
using namespace util;
LRAPlugin::LRAPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request)
  : SolverPlugin(database, trail, request)
  , newVariableNotify(*this)
  , boolTypeIdx(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->booleanType()))
  , realTypeIdx(VariableDatabase::getCurrentDB()->getTypeIndex(NodeManager::currentNM()->realType()))
  , bounds(trail.getSearchContext())
  , trailHead(trail.getSearchContext(), 0)
  , fmRule(database, trail)
  , fmDiseqRule(database, trail)
  {
  Debug("mcsat::lra") << "Starting LRA Plugin" << std::endl;
  //TODO initialize lastDecidedAndUnprocessed????
   addFeature(CAN_PROPAGATE);
   addFeature(CAN_DECIDE_VALUES);
   addNotification(NOTIFY_BACKJUMP);
   VariableDatabase::getCurrentDB() -> addNewVariableListener(&newVariableNotify);
}


LRAPlugin::~LRAPlugin() {}


void LRAPlugin::notifyBackjump(const std::vector<Variable> vars) {
    for (unsigned i =0; i < vars.size(); i++) {
        Variable var = vars[i];
        if (isArithmeticVar(var)) {
              if (lastDecidedAndUnprocessed == var) {
                   lastDecidedAndUnprocessed=Variable::null;
                   continue;
              }
              AssignedWatchManager::remove_iterator iter = watchManager.begin(var);
              while (!iter.done()) {
                  LinearConstraint c = getLinearConstraint(watchManager.getConstraint(*iter));
                  if (c.isFullyAssigned()) {
                      c.setUnit(var);
                  } else if (c.isUnit()) {
                      c.unsetUnit();
                  }
                  iter.next_and_keep();
              }
         }
    }
}

void LRAPlugin::propagate(SolverTrail::PropagationToken& out) {
    unsigned i = trailHead;
    for (; i < d_trail.size() && d_trail.consistent(); i++) {
        Variable v = d_trail[i].var;
        if (isArithmeticVar(v)) {
             if (v == lastDecidedAndUnprocessed) {
                  lastDecidedAndUnprocessed = Variable::null;
             }
             AssignedWatchManager::remove_iterator w = watchManager.begin(v);
             while (d_trail.consistent() && !w.done()) {
                  VariableListReference ref = *w;
                  VariableList list = watchManager.get(ref);
                  if (list.size() > 1 && list[0] == v) {
                      list.swap(0, 1);
                  }
                  bool found = false;
                  for (unsigned j = 2; j < list.size(); j++) {
                      if (!d_trail.hasValue(list[j])) {
                          list.swap(1, j);
                          watchManager.watch(list[1], ref);
                          w.next_and_remove();
                          found = true;
                          break;
                      }
                  }
                  if (!found) {
                       if (d_trail.hasValue(list[0])) {
                           Variable constraintVar = watchManager.getConstraint(ref);
                           getLinearConstraint(constraintVar).setFullyAssigned();
                           if (!d_trail.hasValue(constraintVar)) {
                                unsigned lvl;
                                const LinearConstraint& constraint = getLinearConstraint(constraintVar);
                                bool value = constraint.evaluate(d_trail, lvl); 
                                out (Literal(constraintVar, !value), lvl);
                           } else {
                              //TODO assertion
                           }
                       } else {
                           Variable constraintVar = watchManager.getConstraint(ref);
                           getLinearConstraint(constraintVar).setUnit(list[0]);
                           processUnitConstraint(constraintVar, out);
                       }
                       w.next_and_keep();
                  }
             }
         } else if (isLinearConstraint(v)) {
               if (getLinearConstraint(v).isUnit()) {
                    processUnitConstraint(v, out);
               }
               else {
                        //TODO assert
                }
         }
       
    }
    trailHead = i;
    if (bounds.inConflict()) {
           processConflicts(out);
     }
}


void LRAPlugin::processConflicts(SolverTrail::PropagationToken& out) {
    std::set<Variable> conflictingVars;
    bounds.getVariablesInConflict(conflictingVars);
    std::set<Variable>::const_iterator start = conflictingVars.begin();
    std::set<Variable>::const_iterator end = conflictingVars.end();
    for (;start != end; start++) {
        const BoundInfo& lower = bounds.getLowerBoundInfo(*start);
        const BoundInfo& upper = bounds.getUpperBoundInfo(*start);
        CRef conflict;
        if (BoundInfo::inConflict(lower, upper)) {
            fmRule.start(lower.reason);
            fmRule.resolve(*start, upper.reason);
            conflict = fmRule.finish(out);
        }
        else {
            const fm::DisequalInfo& disequal = bounds.getDisequalInfo(*start, lower.value);
            Literal disEqLiteral (disequal.reason, true);
            conflict = fmDiseqRule.resolveDisequality(*start, lower.reason, upper.reason, disEqLiteral, out);
        }
   }
}
void LRAPlugin::processUnitConstraint(Variable constraint, SolverTrail::PropagationToken& out) {
    LinearConstraint& c = getLinearConstraint(constraint);
    if (d_trail.hasValue(constraint)) {
        BoundingInfo bInfo = c.bound(constraint, d_trail);
        Literal constraintLiteral(constraint, d_trail.isFalse(constraint));
        Variable var = c.getUnitVar();
        if (d_trail.isFalse(constraint)) {
            bInfo.negate();
        }
        BoundInfo b(bInfo.value, bInfo.isStrict(), constraintLiteral);
        fm::CDBoundsModel::update_info u1, u2;
        if (bInfo.isUpperBound()) {
            bounds.updateUpperBound(var, b);
        }
        if (bInfo.isLowerBound()) {
            bounds.updateLowerBound(var, b);
        }
        if (bInfo.kind == kind::DISTINCT) {
            bounds.addDisequality(var, fm::DisequalInfo(bInfo.value, constraint));
        }
    }
} 

void LRAPlugin::decide(SolverTrail::DecisionToken& out, Variable var) {
    if (isArithmeticVar(var)) {
        Rational val = bounds.pick(var, true);
        out(var, NodeManager::currentNM()->mkConst(val), true);
        lastDecidedAndUnprocessed = var;
    }
}

class var_assign_compare { 
    const SolverTrail& trail;
  public:
    var_assign_compare(const SolverTrail& trail)
    : trail(trail) {}
    bool operator () (const Variable& v1, const Variable& v2) {
        bool v1HasVal = trail.hasValue(v1);
        bool v2HasVal = trail.hasValue(v2);
     if (!v1HasVal && !v2HasVal) {
        return v1 < v2;
     }
     if (!v1HasVal) return true;
     if (!v2HasVal) return false;
     return trail.decisionLevel(v1) > trail.decisionLevel(v2);
   }
};

void LRAPlugin::newConstraint(Variable constraint) {
    Debug("mcsat::lra")<<"LRAPLUGIN::newConstraint("<<constraint<<")"<<std::endl;
   
    LinearConstraint linearConstraint;//= getLinearConstraint(constraint); 
    
    LinearConstraint::parse(Literal(constraint, false), linearConstraint);
    std::vector<Variable> vars;
    linearConstraint.getVariables(vars);
    Debug("mcsat::lra") << "wtf"<<std::endl; 
    var_assign_compare cmp(d_trail);
    std::sort(vars.begin(), vars.end(), cmp);
    VariableListReference listRef = watchManager.newListToWatch(vars, constraint);

    //VariableList lsit = watchManager.get(listRef);
    Debug("mcsat::lra") <<"foo"<<std::endl;
    if (vars.size() == 0) {
        return;
    }
    watchManager.watch(vars[0], listRef);
    if (vars.size() > 1) {
        watchManager.watch(vars[1], listRef);
    }
    

    if (constraint.index() >= constraints.size()) {
        constraints.resize(constraint.index() +1);
    }
    constraints[constraint.index()].swap(linearConstraint);
    
    if (!d_trail.hasValue(vars[0])) {
       if (vars.size() == 1 || d_trail.hasValue(vars[1])) {
           if (vars[0] != lastDecidedAndUnprocessed) {
               getLinearConstraint(constraint).setFullyAssigned();
           }
       }
    }
    else {
        if (vars[0] != lastDecidedAndUnprocessed) {
            getLinearConstraint(constraint).setFullyAssigned();
        }
        else {
             getLinearConstraint(constraint).setUnit(vars[0]);
        }
    }
    Debug("mcsat::lra") <<"LRAPlugin::newConstraint done with" <<linearConstraint<<std::endl;

}

std::string LRAPlugin::toString() const {
  return "Linear Real Arithmetic Plugin";
}

void LRAPlugin::outputStatusHeader(std::ostream& out) const {
  out << "status header idk what this does" << std::endl;
}

void LRAPlugin::outputStatusLine(std::ostream& out) const {
  out << "foobar" << std::endl;
}

void LRAPlugin::gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep) {
  Debug("mcsat::lra") << "gcMark" <<std::endl;
}

void LRAPlugin::gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc) {
  Debug("mcsat::lra") <<"gcRelocate"<<std::endl;
}
