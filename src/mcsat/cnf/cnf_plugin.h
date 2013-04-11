#pragma once

#include "mcsat/cnf/tseitin_cnf_stream.h"
#include "mcsat/plugin/solver_plugin.h"
#include "mcsat/rules/input_clause_rule.h"

namespace CVC4 {
namespace mcsat {

class CNFPlugin : public SolverPlugin {

  /** CNF transform of the solver */
  TseitinCnfStream d_cnfStream;

  /** Listener for the output of the cnf stream */
  class CnfListener : public CnfOutputListener {
    /* The outer class */
    CNFPlugin& d_cnfPlugin;
  public:
    CnfListener(CNFPlugin& cnfPlugin)
    : d_cnfPlugin(cnfPlugin) {}
    void newClause(LiteralVector& clause);
  } d_cnfStreamListener;

  friend class CnfListener;

  /** Rule for adding input clauses */
  rules::InputClauseRule d_inputClauseRule;

  /** List of converted clauses */
  std::vector<CRef_Strong> d_convertedClauses;

public:

  /** Constructor */
  CNFPlugin(ClauseDatabase& clauseDb, const SolverTrail& trail, SolverPluginRequest& request);

  /** Notified by the solver of new assertions */
  void notifyAssertion(TNode assertion);

  /** String represtentation */
  std::string toString() const;

};

template class SolverPluginConstructor<CNFPlugin>;

}
}



