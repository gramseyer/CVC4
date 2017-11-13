#pragma once

#include "mcsat/plugin/solver_plugin.h"

#include "mcsat/lra/linear_constraint.h"
#include "mcsat/variable/variable_table.h"
#include "mcsat/fm/bounds_model.h"
#include "mcsat/rules/fm_rule_copy.h"
#include "mcsat/util/assigned_watch_manager.h"
namespace CVC4 {
namespace mcsat {

class LRAPlugin : public SolverPlugin {
  
 
    class NewVariableNotify : public VariableDatabase::INewVariableNotify {
        LRAPlugin& plugin;
      public:
        NewVariableNotify(LRAPlugin& plugin) 
        : INewVariableNotify(false)
        , plugin(plugin) {}
        
        void newVariable(Variable v) {
            switch(v.getNode().getKind()) {
                 case kind::LT:
                 case kind::LEQ:
                 case kind::GT:
                 case kind::GEQ:
                 case kind::EQUAL:
                     plugin.newConstraint(v);
                 default:
                     return;
             }
        }
    } newVariableNotify;

    size_t boolTypeIdx;
    size_t realTypeIdx;
    std::vector<lra::LinearConstraint> constraints;
//    std::vector<unit_info> status;
    fm::CDBoundsModel bounds;
    Variable lastDecidedAndUnprocessed;
    context::CDO<size_t> trailHead;
    rules::FourierMotzkinRuleCopy fmRule;
    rules::FourierMotzkinRuleDiseqCopy fmDiseqRule;
    util::AssignedWatchManager watchManager;
  public:
    LRAPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request);
    ~LRAPlugin();

    void propagate(SolverTrail::PropagationToken& out);
    void notifyBackjump(const std::vector<Variable> vars); 
    void decide(SolverTrail::DecisionToken& out, Variable var);
    void newConstraint(Variable constraint);

    std::string toString() const;
    void outputStatusHeader(std::ostream& out) const;
    void outputStatusLine(std::ostream& out) const;
    void gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep);
    void gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc);
    
    void processUnitConstraint(Variable constraint, SolverTrail::PropagationToken& out);
    void processConflicts(SolverTrail::PropagationToken& out);
    bool isLinearConstraint(Variable var) const {
        return var.typeIndex() == boolTypeIdx
            && var.index() < constraints.size() && !constraints[var.index()].isNull();
    }
    bool isArithmeticVar(Variable var) const {
        return var.typeIndex() == realTypeIdx;
    }
    lra::LinearConstraint& getLinearConstraint(Variable var) {
        return constraints[var.index()];
    }
};


MCSAT_REGISTER_PLUGIN(LRAPlugin);



}
}

