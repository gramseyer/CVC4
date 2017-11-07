#pragma once

#include "mcsat/plugin/solver_plugin.h"

#include "mcsat/lra/linear_constraint.h"

namespace CVC4 {
namespace mcsat {

class LRAPlugin : public SolverPlugin {
  

    size_t boolTypeIdx;
    std::vector<lra::LinearConstraint> constraints;

  public:
    LRAPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request);
    ~LRAPlugin();

    void propagate(SolverTrail::PropagationToken& out);
    
    void decide(SolverTrail::DecisionToken& out, Variable var);
    void newConstraint(Variable constraint);

    std::string toString() const;
    void outputStatusHeader(std::ostream& out) const;
    void outputStatusLine(std::ostream& out) const;
    void gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep);
    void gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc);
    

    bool isLinearConstraint(Variable var) const {
        return var.typeIndex() == boolTypeIdx
            && var.index() < constraints.size() && !constraints[var.index()].isNull();
    }
};


MCSAT_REGISTER_PLUGIN(LRAPlugin);



}
}

