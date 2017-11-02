#include "mcsat/lra/lra_plugin.h"

using namespace CVC4;
using namespace mcsat;
//using namespace lra;

LRAPlugin::LRAPlugin(ClauseDatabase& database, const SolverTrail& trail, SolverPluginRequest& request)
  : SolverPlugin(database, trail, request)
  {
  Debug("mcsat::lra") << "Starting LRA Plugin" << std::endl;
}


LRAPlugin::~LRAPlugin() {}


void LRAPlugin::propagate(SolverTrail::PropagationToken& out) {
}

void LRAPlugin::decide(SolverTrail::DecisionToken& out, Variable var) {
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
