#pragma once

#include "expr/node.h"
#include "mcsat/variable/variable.h"
#include "mcsat/clause/literal.h"
#include "mcsat/fm/bound_info.h"

namespace CVC4 {
namespace mcsat {

class SolverTrail;

namespace lra {

typedef std::pair<Variable, Rational> var_rational_pair;

typedef std::vector<var_rational_pair> var_rational_pair_vector;

//copied from bm/LinearConstraint.cpp
struct BoundingInfo {
  Rational value;
  Kind kind;

  BoundingInfo(Rational value, Kind kind)
  : value(value), kind(kind) {}

  BoundingInfo()
  : value(0), kind(kind::UNDEFINED_KIND)
  {}

  void swap(BoundingInfo& other) {
    std::swap(value, other.value);
    std::swap(kind, other.kind);
  }

  void clear() {
    value = 0;
    kind = kind::UNDEFINED_KIND;
  }

  void negate();

  /** Is this a lower bound */
  bool isLowerBound() const {
    switch(kind) {
    case kind::GT:
    case kind::GEQ:
    case kind::EQUAL:
      return true;
    default:
      return false;
    }
  }

  /** Is this an upper bound */
  bool isUpperBound() const {
    switch(kind) {
    case kind::LT:
    case kind::LEQ:
    case kind::EQUAL:
      return true;
    default:
      return false;

    }
  }

  /** Is this a strict bound */
  bool isStrict() const {
    return kind == kind::LT || kind == kind::GT;
  }
};

class LinearConstraint {
    Variable unitVar;
    bool unit;



    var_rational_pair_vector coeffs;
    
    Kind kind;
    
    static void normalize(var_rational_pair_vector& coefficients);
    static bool parse(TNode constraint, Rational mult, var_rational_pair_vector& coefficientMap);

  public:
    LinearConstraint(): unit(false) {}
    BoundingInfo bound(Variable x, const SolverTrail& trail) const;    
    

    static Kind negateKind(Kind k);
    Literal getLiteral(const SolverTrail& trail) const;
    bool isUnit() const {
        return unit && !unitVar.isNull();
    }
    typedef var_rational_pair_vector::const_iterator const_iterator;
    void setUnit(Variable v) {
        unitVar = v;
        unit = true;
     }
    Variable getUnitVar() {
       return unitVar;
    }
    
    void setFullyAssigned() {
       unitVar = Variable::null;
       unit = true;
    }
    bool isFullyAssigned() {
       return unit && unitVar.isNull();
    }
    void unsetUnit() {
       unitVar = Variable::null;
       unit = false;
    }
    void clear();
    void multiply(Rational r);
    void add(const LinearConstraint& other, Rational r);
    void flipEquality();
    void splitDisequality(Variable v, LinearConstraint& o);
    Rational getCoefficient(Variable v);
    Kind getKind() {
        return kind;
    }
    static Kind flipKind(Kind k);
    bool evaluate(const SolverTrail& trail, unsigned& level) const;
    bool evaluate(const SolverTrail& trail) const {
        unsigned level;
        return evaluate(trail, level);
    }
    int getEvaluationLevel(const SolverTrail& trail) {
        unsigned level;
        evaluate(trail, level);
        return level;
    }
    static bool parse(Literal constraint, LinearConstraint& out);
    void toStream(std::ostream& out) const;
    bool isNull() const {
        return kind == kind::LAST_KIND;
    }
    void swap(LinearConstraint& c);
    void getVariables(std::vector<Variable> vars) const;
};

inline std::ostream& operator << (std::ostream& out, const LinearConstraint& c) {
     c.toStream(out);
     return out;
}

}
}
}
