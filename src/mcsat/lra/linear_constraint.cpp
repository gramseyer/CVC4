#include "mcsat/lra/linear_constraint.h"
#include "mcsat/variable/variable_db.h"
#include "mcsat/solver_trail.h"
#include "theory/rewriter.h"


using namespace CVC4;
using namespace mcsat;
using namespace lra;

struct normalize_cmp {
    bool operator () (const var_rational_pair& p1, var_rational_pair& p2) {
        return p1.first < p2.first;
    }
};



Kind LinearConstraint::flipKind(Kind k) {
    switch (k) {
        case kind::GT:
            return kind::LT;
        case kind::GEQ:
            return kind::LEQ;
        case kind::LT:
            return kind::GT;
        case kind::LEQ:
            return kind::GEQ;
        default:
            return k;
    }
}
Literal LinearConstraint::getLiteral(const SolverTrail& trail) const {
    NodeManager* nm = NodeManager::currentNM();
    VariableDatabase* db = VariableDatabase::getCurrentDB();
    Node s;
    if (coeffs.size() > 1) {
        NodeBuilder<> builder(kind::PLUS);
        for (const_iterator it = coeffs.begin(); it != coeffs.end(); it++) {
             if (it -> first.isNull()) {
                  builder << nm -> mkConst<Rational>(it->second);
             } else {
                  Node x;
                  if (trail.hasValue(it->first) && trail.decisionLevel(it->first) == 0) {
                      x = trail.value(it->first);
                  } else {
                      x = it->first.getNode();
                  }
                  Node mult = nm->mkConst<Rational>(it->second);
                  builder << nm->mkNode(kind::MULT, mult, x);
             }
         }
         s= builder;
     } else {
         s = nm->mkConst<Rational>(coeffs.begin()->second);
     }
     Node n =  nm->mkNode(kind, s, nm->mkConst<Rational>(0));
     Node normalized = theory::Rewriter::rewrite(n);
     TNode atom = (normalized.getKind() == kind::NOT) ? normalized[0] : normalized;
     return Literal(db->getVariable(atom), normalized.getKind() == kind::NOT);
}
void LinearConstraint::clear() {
    coeffs.clear();
    kind = kind::LAST_KIND;
    coeffs.push_back(var_rational_pair(Variable::null, 0));
}

void LinearConstraint::multiply(Rational r) {
    for (var_rational_pair_vector::iterator it = coeffs.begin(); it != coeffs.end(); it++) {
        it -> second = it->second * r;
    }
}

Rational LinearConstraint::getCoefficient(Variable v) {
    const_iterator it = coeffs.begin();
    for (; it != coeffs.end(); it++) {
        if (it->first == v) {
            return it->second;
        }
    }
    return 0;
}

void LinearConstraint::splitDisequality(Variable v, LinearConstraint& o) {
    o.coeffs = coeffs;
    o.kind = kind::DISTINCT;
    if (getCoefficient(v) < 0) {
       flipEquality();
    }
    else {
        o.flipEquality();
    }
    kind = kind::GT;
    o.kind = kind::GT; // THIS METHOD IS JANK
} 

void LinearConstraint::flipEquality() {
    var_rational_pair_vector::iterator it = coeffs.begin();
    var_rational_pair_vector::iterator it_end = coeffs.end();
    for (; it != it_end; it++) {
        it->second = it->second * -1;
    }
}

void LinearConstraint::add(const LinearConstraint& c, Rational r) {
    switch(kind) {
        case kind::EQUAL:
            kind = c.kind;
            break;
        case kind::GEQ:
            if (c.kind == kind::GT) {
               kind = kind::GT;
            }
            break;
        default:
            kind = kind::GT;
            //nothing
     }
     for (const_iterator it = c.coeffs.begin(); it != c.coeffs.end(); it++) {
         coeffs.push_back(var_rational_pair(it->first, it->second * r));
      }
    normalize(coeffs);
}

void BoundingInfo::negate() {
    kind = LinearConstraint::negateKind(kind);
}
BoundingInfo LinearConstraint::bound(Variable x, const SolverTrail& trail) const {
    Rational a, sum;
    const_iterator iterator = coeffs.begin();
    const_iterator iterator_end = coeffs.end();
    for (;iterator != iterator_end; iterator++) {
        Variable v = iterator -> first;
        if (v.isNull()) {
             sum  += iterator-> second;
        }
        else if (v == x) {
             a = iterator -> second;
        }
        else {
             sum += iterator -> second * trail.value(v).getConst<Rational>();
        }
    }
    Kind k = kind;
    if (a<0) {
        k = flipKind(k);
    }
    return BoundingInfo (-sum/a, k);
}

void LinearConstraint::normalize(var_rational_pair_vector& coefficients) {
    normalize_cmp cmp;
    std::sort(coefficients.begin(), coefficients.end(), cmp);
    unsigned head = 0;
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
    Rational m = 1;
    out.kind = n.getKind();
    if (constraint.isNegated()) {
        out.kind = negateKind(out.kind);
        Debug("mcsat::lra") <<"CONSTRAINT IS NEGATED WHAT IS THIS"<<std::endl;
    }
    if (out.kind == kind::LT || out.kind == kind::LEQ) {
        m =-m;
        out.kind = flipKind(out.kind);
    }
    
    parse(n[0], m, out.coeffs);
    parse(n[1], -m, out.coeffs);
    normalize(out.coeffs);
    Debug("mcsat::lra") << "lra linear constraint parse (" << constraint << ") => " << out <<std::endl;
   return false;
}

Kind LinearConstraint::negateKind(Kind k) {
   switch(k) {
       case kind::LT:
           return kind::GEQ;
       case kind::LEQ:
           return kind::GT;
       case kind::GT:
           return kind::LEQ;
       case kind::GEQ:
           return kind::LT;
       case kind::EQUAL:
           return kind::DISTINCT;
       case kind::DISTINCT:
           return kind::EQUAL;
       default:
           Unreachable();
   }
   Unreachable();
   return kind::LAST_KIND;
}

bool LinearConstraint::parse(TNode t, Rational m, var_rational_pair_vector& coefficientMap) {
    VariableDatabase& db = *VariableDatabase::getCurrentDB();
     
    switch(t.getKind()) {
        case kind::CONST_RATIONAL:
             coefficientMap.push_back(var_rational_pair(Variable::null, m * t.getConst<Rational>()));
             break;
        case kind::MULT:
             Assert(t.getNumChildren() == 2 && t[0].isConst());
             return parse(t[1], m*t[0].getConst<Rational>(), coefficientMap);
        case kind::PLUS:
             for (unsigned i = 0; i < t.getNumChildren(); i++) {
                 parse(t[i], m, coefficientMap);
             }
             break;
        case kind::MINUS:
             Assert(t.getNumChildren() == 2);
             parse(t[0], m, coefficientMap);
             parse(t[1], -m, coefficientMap);
             break;
        case kind::UMINUS:
             parse(t[0], -m, coefficientMap);
             break;
        default:
            Variable v = db.getVariable(t);
            coefficientMap.push_back(var_rational_pair(v, m));
            break;
    }
    return true;
}

void LinearConstraint::toStream(std::ostream& out) const {
    out<<"LinConstr[" <<kind <<", ";
    var_rational_pair_vector::const_iterator it = coeffs.begin();
    while (it != coeffs.end()) {
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

bool LinearConstraint::evaluate(const SolverTrail& trail, unsigned& level) const {
    Debug("mcsat::lra") << "Evaluating" << this <<std::endl;
    Rational lhsValue = coeffs[0].second;
    for (unsigned i = 1; i < coeffs.size(); i++) {
        Variable var = coeffs[i].first;
        Assert(trail.hasValue(var));
        lhsValue += trail.value(var).getConst<Rational>() * coeffs[i].second;
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
    default:
        Unreachable();
    }
    Debug("mcsat::lra") << "evaluate shouldn't reach here"<<std::endl;
    Unreachable();
    return false;
}

void LinearConstraint::swap(LinearConstraint& c) {
    coeffs.swap(c.coeffs);
    std::swap(kind, c.kind);
    std::swap(unit, c.unit);
    std::swap(unitVar, c.unitVar);
}

void LinearConstraint::getVariables(std::vector<Variable> vars) const {
    var_rational_pair_vector::const_iterator iter = coeffs.begin();
    while (iter != coeffs.end()) {
        if (!(iter->first.isNull())) {
             vars.push_back (iter->first);
        }
        iter++;
    }
}
