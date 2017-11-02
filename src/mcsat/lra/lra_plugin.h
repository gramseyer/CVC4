#pragma once

#include "mcsat/plugin/solver_plugin.h"



namespace CVC4 {
namespace mcsat {

class LRAPlugin : public SolverPlugin {
  

  public:
    LRAPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request);
    ~LRAPlugin();

    void propagate(SolverTrail::PropagationToken& out);
    
    void decide(SolverTrail::DecisionToken& out, Variable var);
    

    std::string toString() const;
    void outputStatusHeader(std::ostream& out) const;
    void outputStatusLine(std::ostream& out) const;
    void gcMark(std::set<Variable>& varsToKeep, std::set<CRef>& clausesToKeep);
    void gcRelocate(const VariableGCInfo& vReloc, const ClauseRelocationInfo& cReloc);

};


MCSAT_REGISTER_PLUGIN(LRAPlugin);



}
}

