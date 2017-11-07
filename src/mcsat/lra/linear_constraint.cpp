#include "mcsat/lra/linear_constraint.h"
#include "mcsat/variable/variable_db.h"
#include "mcsat/solver_trail.h"



using namespace CVC4;
using namespace mcsat;
using namespace lra;

struct normalize_cmp {
    bool operator () (const var_rational_pair& p1, var_rational_pair& p2) {
        return p1.first < p2.first;
    }
};

void LinearConstraint::normalize(var_rational_pair_vector& coefficients) {
    normalize_cmp cmp;
    std::sort(coefficients.begin(), coefficients.end(), cmp);
    unsigned first = 0;
    for (unsigned  i =1; i<coefficients.size(); i++) {
        if (coefficients[head].first != coefficients[i].first) {
            if (coefficients[head].first.isNull() || coefficients[head].second != 0) {
                head++;
            }
            coefficients[head] = coefficients[i];
        } else {
            coefficients[head].second += coefficients[i].second;
        }
    }
    coefficients.resize(head+1);
}


bool LinearConstraint::parse(Literal constraint, LinearConstraint& out) {
    Debug ("mcsat::lra") << "lra LinearConstraint parse(" << constraint << ")"<<std::endl;
    

    Variable var = constraint.getVariable();
    TNode n = var.getNode();
    Rational mult = 1;
    out.kind = node.getKind();
    if (constraint.isNegated()) {
        Debug("mcsat::lra") <<"CONSTRAINT IS NEGATED WHAT IS THIS"<<std::endl;
    }
    if (kind == kind::LT || kind == kind::LEQ) {
        m =-m;
        out.kind = flipKind(kind);
    }
    
    parse(node[0], m, out.coeffs);
    parse(node[1], -m, out.coeffs);
    normalize(out.coeffs);
    Debug("mcsat::lra") << "lra linear constraint parse (" << constraint << ") => " << out <<std::endl;
}

bool LinearConstraint::parse(TNode t, Rational m, var_rational_pair_vector& coefficientMap) {
    VariableDatabase& db = *VariableDatabase::getCurrentDB();
     
    switch(constraint.getKind()) {
        case kind::CONST_RATIONAL:
             coefficientMap.push_back(var_rational_pair(Variable::null, m * t.getConst<Rational>()));
             break;
        case kind::MULT:
             Assert(t.getNumChildren() == 2 && t[0].isConst());
             return parse(t[1], m*t[0].getConst<Rational>(), coefficientMap);
        case kind::PLUS:
        case kind::MINUS:
        case kind::UMINUS:
        default:
            Variable v = db.getVariable(term);
            coefficientMap.push_back(var_rational_pair(v, m));
            break;
    }
    return true;
}

void LinearConstraint::toStream(std::ostream& out) const {
    out<<"LinConstr[" <<kind <<", ";
    var_rational_pair_vector::const iterator it = coefficients.begin();
    while (it != coefficients.end()) {
        out << "+";
        if (it -> first.isNull()) {
            out<< it->second;
        }
        else {
            out << "("<<  it->second<<"*"<<it->first<<")";
        }
        ++it;
    }
    out <<"]";
}

bool LinearConstraint::evaluate(const SolverTrail& trail, unsigned& level) {
    Rational lhsValue = coefficients[0].second;
    for (unsigned i = 1; i < coefficients.size(); i++) {
        Variable var = coefficients[i].first;
        Assert(trail.hasValue(var));
        lhsValue += trail.value(var).getConst<Rational>() * coefficients[i].second;
    }
    switch(kind) {
    case kind::LT:
        return lhsValue < 0;
    case kind::LEQ:
        return lhsValue <= 0;
    case kind::GT:
        return lhsValue > 0;
    case kind::GEQ:
        return lhsValue >= 0;
    case kind::EQUAL:
        return lhsValue == 0;
    case kind::DISTINCT:
        return lhsValue != 0;
    }
    Debug("mcsat::lra") << "evaluate shouldn't reach here"<<std::endl;
    Unreachable();
    return false;
}








