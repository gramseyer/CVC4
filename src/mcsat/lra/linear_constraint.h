#pragma once

namespace CVC4 {
namespace mcsat {

//class SolverTrail;

namespace lra {

typedef std::pair<Variable, Rational> var_rational_pair;

typedef std::vector<var_rational_pair> var_rational_pair_vector;

class LinearConstraint {
    var_rational_pair_vector coeffs;
    Kind kind;
   
    static void normalize(var_rational_pair_vector& coefficients);
    static bool parse(TNode constraint, Rational mult, var_rational_pair_vector& coefficientMap);

  public:
    LinearConstraint();
    

    bool evaluate(const SolverTrail& trail, unsigned& level) const;
    bool evaluate(const SolverTrail& trail) const {
        unsigned level;
        return evaluate(trail, level);
    }
    static bool parse(Literal constraint, LinearConstraint& out);
    void toStream(std::ostream out) const;
    bool isNull() const {
        return kind == kind::LAST_KIND;
    }
};
}
}
}
