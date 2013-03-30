#pragma once

#include "expr/node.h"
#include "context/cdo.h"
#include "mcsat/variable/variable.h"

#include <iostream>
#include <boost/integer_traits.hpp>

namespace CVC4 {
namespace mcsat {
namespace fm {
  
/** Map from variables to coefficients, null is the constant term */
typedef std::hash_map<Variable, Rational, VariableHashFunction> coefficient_map;

/** Bound information */
struct BoundInfo {
  /** The value of the bound */
  Rational value;
  /** Is the bound strict (<,>) */
  bool strict;
  /** What is the reason for this bound */
  Variable reason;
};

/** Context-dependent bounds model */
class CDBoundsModel : public context::ContextNotifyObj {

  /** Index of the bound in the bounds vector */
  typedef unsigned BoundIndex;
  
  /** Null bound index */
  static const BoundIndex null = boost::integer_traits<BoundIndex>::const_max;
  
  /** Map from variables to their bounds */
  typedef std::hash_map<Variable, BoundIndex, VariableHashFunction> bound_map;
    
  /** Lower bounds */
  bound_map d_lowerBounds;
  
  /** Upper bounds */
  bound_map d_upperBounds;
  
  /** Bound update trail */
  std::vector<BoundInfo> d_boundTrail;

  /** Information for undoing */
  struct UndoInfo {
    /** Which variable */
    Variable var;
    /** Index of the previous bound (null if none) */
    BoundIndex prev;
    /** Is it a lower bound */
    bool isLower;
    
    UndoInfo() {}
    
    UndoInfo(Variable var, BoundIndex prev, bool isLower)
    : prev(prev), isLower(isLower) {}      
  };
  
  /** Bound update trail */
  std::vector<UndoInfo> d_boundTrailUndo;
  
  /** Count of lower bound updates */
  context::CDO<unsigned> d_boundTrailSize;
  
  /** Update to the apropriate context state */
  void contextNotifyPop();
  
public:
  
  ~CDBoundsModel() throw(AssertionException) {}
  
  /** Construct it */
  CDBoundsModel(context::Context* context);
  
  /** Update the lower bound */
  void updateLowerBound(Variable var, const BoundInfo& info);

  /** Update the upper bound */
  void updateUpperBound(Variable var, const BoundInfo& info);

  /** Does the variable have a lower bound */
  bool hasLowerBound(Variable var) const;

  /** Does the variable have an upper bound */
  bool hasUpperBound(Variable var) const;
  
  /** Get the current lower bound info */
  const BoundInfo& getLowerBoundInfo(Variable var) const;

  /** Get the current upper bound info */
  const BoundInfo& getUpperBoundInfo(Variable var) const;
  
};

/**
 * A generic linear constraint (t op 0) is just a map from variables to coefficients
 * representing a term t, and a kind of constraint op.
 */ 
struct LinearConstraint {
  
  /** Map from variables to coefficients */
  coefficient_map coefficients;
  /** The kind of constraint >=, ... */
  Kind kind;

  /** Default constructor */
  LinearConstraint(): kind(kind::LAST_KIND) {
    coefficients[Variable::null] = 0;
  }

  /** Get the variables of this constraint */
  void getVariables(std::vector<Variable>& vars);

  /** Swap with the given constraint */
  void swap(LinearConstraint& c);
};

/** Map froom variables to constraints */
typedef std::hash_map<Variable, LinearConstraint, VariableHashFunction> constraints_map;

inline std::ostream& operator << (std::ostream& out, const LinearConstraint& c) {
  fm::coefficient_map::const_iterator it = c.coefficients.begin();
  fm::coefficient_map::const_iterator it_end = c.coefficients.end();
  bool first = true;
  while (it != it_end) {
    out << (first ? "" : "+");
    if (it->first.isNull()) {
      out << it->second;
    } else {
      out << "(" << it->second << "*" << it->first << ")";
    }
    first = false;
    ++ it;
  }
  return out;
}

} // fm namespace
}
}


