#include "mcsat/fm/linear_constraint.h"
#include "mcsat/variable/variable_db.h"
#include "mcsat/solver_trail.h"

#include "theory/rewriter.h"

using namespace CVC4;
using namespace mcsat;
using namespace fm;

struct variable_search_cmp {
  // Just compare the variables
  bool operator () (const var_rational_pair& p1, const var_rational_pair& p2) {
    return p1.first < p2.first;
  }
};

void LinearConstraint::normalize(var_rational_pair_vector& coefficients) {
  // Sort the coefficients
  variable_search_cmp cmp;
  std::sort(coefficients.begin(), coefficients.end(), cmp);

  // Gather all same coefficients
  unsigned head = 0;
  for (unsigned i = 1; i < coefficients.size(); ++ i) {
    if (coefficients[head].first != coefficients[i].first) {
      if (coefficients[head].first.isNull() || coefficients[head].second != 0) {
        // If the new element is with different variable, move keep
        coefficients[++ head] = coefficients[i];
      } else {
        // Canceled out, so we overwrite it
        coefficients[head] = coefficients[i];
      }
    } else {
      // If we're with the same variable, just add up
      coefficients[head].second += coefficients[i].second;
    }
  }
  coefficients.resize(head + 1);
}

void LinearConstraint::getVariables(std::vector<Variable>& vars) const {
  var_rational_pair_vector::const_iterator it = d_coefficients.begin();
  var_rational_pair_vector::const_iterator it_end = d_coefficients.end();
  while (it != it_end) {
    Variable var = it->first;
    if (!var.isNull()) {
      vars.push_back(var);
    }
    ++ it;
  }
}

void LinearConstraint::swap(LinearConstraint& c) {
  d_coefficients.swap(c.d_coefficients);
  d_variablesValueCache.swap(c.d_variablesValueCache);
  std::swap(d_kind, c.d_kind);
  std::swap(d_constraintValueCache, c.d_constraintValueCache);
}

int LinearConstraint::getEvaluationLevel(const SolverTrail& trail) const {

  unsigned level = 0;

  // Get the max level of all the variables
  const_iterator it = begin();
  const_iterator it_end = end();
  for (; it != it_end; ++ it) {
    Variable var = it->first;
    if (!var.isNull()) {
      if (trail.hasValue(var)) {
        level = std::max(level, trail.decisionLevel(var));
      } else {
        return -1;
      }
    }
  }

  return level;
}

bool LinearConstraint::evaluate(const SolverTrail& trail, unsigned& level) const {

  level = 0;

  // Check if the cache is valid
  bool cacheValid = true;
  if (!d_variablesValueCache.empty()) {
    Assert(d_variablesValueCache.size() == d_coefficients.size());
    for (unsigned i = 0; i < d_variablesValueCache.size(); ++ i) {
      Variable var = d_coefficients[i].first;
      if (!var.isNull()) {
        level = std::max(level, trail.decisionLevel(var));
        if (trail.value(var).getConst<Rational>() != d_variablesValueCache[i]) {
          cacheValid = false;
        }
      }
    }
  } else {
    cacheValid = false;
  }

  if (cacheValid) {
    return d_constraintValueCache;
  }

  // Make sure enough value for the cache
  const_cast<LinearConstraint*>(this)->d_variablesValueCache.clear();

  Rational lhsValue;
  const_iterator it = begin();
  const_iterator it_end = end();
  for (; it != it_end; ++ it) {
    Variable var = it->first;
    if (var.isNull()) {
      lhsValue += it->second;
      const_cast<LinearConstraint*>(this)->d_variablesValueCache.push_back(0);
    } else {
      Assert(trail.hasValue(var));
      level = std::max(level, trail.decisionLevel(var));
      Rational value = trail.value(var).getConst<Rational>();
      const_cast<LinearConstraint*>(this)->d_variablesValueCache.push_back(value);
      lhsValue += value * it->second;
    }
  }

  Assert(d_variablesValueCache.size() == d_coefficients.size());

  int sgn = lhsValue.sgn();
  bool value;
  switch (d_kind) {
  case kind::LT:
    value = (sgn < 0);
    break;
  case kind::LEQ:
    value = (sgn <= 0);
    break;
  case kind::GT:
    value = (sgn > 0);
    break;
  case kind::GEQ:
    value = (sgn >= 0);
    break;
  case kind::EQUAL:
    value = (sgn == 0);
    break;
  case kind::DISTINCT:
    value = (sgn != 0);
    break;
  default:
    Unreachable();
    break;
  }

  const_cast<LinearConstraint*>(this)->d_constraintValueCache = value;

  return value;
}


bool LinearConstraint::parse(Literal constraint, LinearConstraint& out) {

  Debug("mcsat::linear") << "LinearConstraint::parse(" << constraint << ")" << std::endl;

  out.clear();

  Variable var = constraint.getVariable();
  TNode node = var.getNode();

  // Multiplier for LHS and RHS
  Rational m = 1;

  // Set the kind of the constraint
  out.d_kind = node.getKind();
  if (constraint.isNegated()) {
    out.d_kind = negateKind(out.d_kind);
  }

  // If < or <=, multiply with -1
  if (out.d_kind == kind::LT || out.d_kind == kind::LEQ) {
    m = -m;
    out.d_kind = flipKind(out.d_kind);
  }
  
  // Parse the LHS
  bool linear = parse(node[0],  m, out.d_coefficients);

  // If linear parse the RHS
  if (linear) {
    linear = parse(node[1], -m, out.d_coefficients);
  }

  // Normalize
  normalize(out.d_coefficients);

  Debug("mcsat::linear") << "LinearConstraint::parse(" << constraint << ") => " << out << std::endl;

  return linear;
}

bool LinearConstraint::parse(TNode term, Rational mult, var_rational_pair_vector& coefficientMap) {

  Debug("mcsat::linear") << "LinearConstraint::parse(" << term << ")" << std::endl;

  VariableDatabase& db = *VariableDatabase::getCurrentDB();

  switch (term.getKind()) {
    case kind::CONST_RATIONAL:
      coefficientMap.push_back(var_rational_pair(Variable::null, mult*term.getConst<Rational>()));
      break;
    case kind::VARIABLE:
    case kind::SKOLEM: {
      Assert(db.hasVariable(term));
      Variable var = db.getVariable(term);
      coefficientMap.push_back(var_rational_pair(var, mult));
      break;
    }
    case kind::MULT: {
      if (term.getNumChildren() == 2 && term[0].isConst()) {
        return parse(term[1], mult*term[0].getConst<Rational>(), coefficientMap);
      } else {
        return false;
      }
      break;
    }
    case kind::PLUS:
      for (unsigned i = 0; i < term.getNumChildren(); ++ i) {
        bool linear = parse(term[i], mult, coefficientMap);
        if (!linear) {
          return false;
        }
      }
      break;
    case kind::MINUS: {
      bool linear;
      linear = parse(term[0],  mult, coefficientMap);
      if (!linear) {
        return false;
      }
      return parse(term[1], -mult, coefficientMap);
      break;
    }
    case kind::UMINUS:
      return parse(term[0], -mult, coefficientMap);
      break;
    default:
      Unreachable();
  }

  return true;
}

void LinearConstraint::toStream(std::ostream& out) const {
  out << "LinearConstraint[" << d_kind << ", ";
  var_rational_pair_vector::const_iterator it = d_coefficients.begin();
  var_rational_pair_vector::const_iterator it_end = d_coefficients.end();
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
  out << "]";
}

Kind LinearConstraint::negateKind(Kind kind) {

  switch (kind) {
  case kind::LT:
    // not (a < b) = (a >= b)
    return kind::GEQ;
  case kind::LEQ:
    // not (a <= b) = (a > b)
    return kind::GT;
  case kind::GT:
    // not (a > b) = (a <= b)
    return kind::LEQ;
  case kind::GEQ:
    // not (a >= b) = (a < b)
    return kind::LT;
  case kind::EQUAL:
    // not (a = b) = (a != b)
    return kind::DISTINCT;
  default:
    Unreachable();
    break;
  }

  return kind::LAST_KIND;
}

Kind LinearConstraint::flipKind(Kind kind) {

  switch (kind) {
  case kind::LT:
    return kind::GT;
  case kind::LEQ:
    return kind::GEQ;
  case kind::GT:
    return kind::LT;
  case kind::GEQ:
    return kind::LEQ;
  default:
    return kind;
  }
}

Rational LinearConstraint::getCoefficient(Variable var) const {
  variable_search_cmp cmp;
  var_rational_pair_vector::const_iterator find = std::lower_bound(d_coefficients.begin(), d_coefficients.end(), var_rational_pair(var, 0), cmp);
  if (find == d_coefficients.end()) {
    return 0;
  } else if (find->first == var) {
    return find->second;
  } else {
    return 0;
  }
}

void LinearConstraint::multiply(Rational c) {
  Assert(c > 0);

  Debug("mcsat::linear") << "LinearConstraint::multiply(): " << c << " * " << *this << std::endl;

  var_rational_pair_vector::iterator it = d_coefficients.begin();
  var_rational_pair_vector::iterator it_end = d_coefficients.end();
  for (; it != it_end; ++ it) {
    it->second *= c;
  }

  Debug("mcsat::linear") << "LinearConstraint::multiply(): = " << *this << std::endl;
}

void LinearConstraint::flipEquality() {
  Assert(d_kind == kind::EQUAL || d_kind == kind::DISTINCT);
  
  Debug("mcsat::linear") << "LinearConstraint::flipEquality(): " << -1 << " * " << *this << std::endl;

  var_rational_pair_vector::iterator it = d_coefficients.begin();
  var_rational_pair_vector::iterator it_end = d_coefficients.end();
  for (; it != it_end; ++ it) {
    it->second *= -1;
  }

  Debug("mcsat::linear") << "LinearConstraint::flipEquality(): = " << *this << std::endl;
}

void LinearConstraint::splitDisequality(Variable x, LinearConstraint& other) {
  Assert(d_kind == kind::DISTINCT);
  Assert(!x.isNull() && getCoefficient(x) != 0);

  d_variablesValueCache.clear();
  other.d_variablesValueCache.clear();

  // Make a copy
  other.d_coefficients = d_coefficients;
  other.d_kind = kind::DISTINCT;

  // Setup the coefficients
  if (getCoefficient(x) < 0) {
    flipEquality();
  } else {
    other.flipEquality();
  }

  // The kinds
  d_kind = kind::GT;
  other.d_kind = kind::GT;
}
  
void LinearConstraint::add(const LinearConstraint& other, Rational c) {

  Debug("mcsat::linear") << "LinearConstraint::add(): " << *this << " + " << c << " * " << other << std::endl;

  d_variablesValueCache.clear();

  // Figure out the resulting kind
  switch (d_kind) {
  case kind::EQUAL:
    // The result is whatever the other one is
    d_kind = other.d_kind;
    break;
  case kind::GT:
    // GT + any other constraint is GT, so we stay the same
    break;
  case kind::GEQ:
    // GEQ + EQ  = GEQ
    // GEQ + GEQ = GEQ
    // GEQ + GT  = GT
    if (other.d_kind == kind::GT) {
      d_kind = kind::GT;
    }
    break;
  default:
    Unreachable();
  }

  // Add up the terms
  const_iterator it = other.begin();
  const_iterator it_end = other.end();
  for (; it != it_end; ++ it) {
    d_coefficients.push_back(var_rational_pair(it->first, c*it->second));
  }

  // Normalize
  normalize(d_coefficients);

  Debug("mcsat::linear") << "LinearConstraint::add(): = " << *this << std::endl;

}

Literal LinearConstraint::getLiteral() const {

  Debug("mcsat::linear") << "LinearConstraint::getLiteral(): " << *this << std::endl;

  Assert(d_coefficients.size() >= 1);
  
  NodeManager* nm = NodeManager::currentNM();
  VariableDatabase* db = VariableDatabase::getCurrentDB();

  // Construct the sum
  Node sum;
  if (d_coefficients.size() > 1) {
    NodeBuilder<> sumBuilder(kind::PLUS);
    const_iterator it = begin();
    const_iterator it_end = end();
    for (; it != it_end; ++ it) {
      Variable x = it->first;
      Rational a = it->second;
      if (x.isNull()) {
        sumBuilder << nm->mkConst<Rational>(a);
      } else {
        Node xNode = x.getNode();
        Node aNode = nm->mkConst<Rational>(a);
        sumBuilder << nm->mkNode(kind::MULT, aNode, xNode);
      }
    }
    sum = sumBuilder;
  } else {
    // We only have the constant
    Assert(d_coefficients.begin()->first.isNull());
    sum = nm->mkConst<Rational>(d_coefficients.begin()->second);
  }

  // Construct the constraint 
  Node node = nm->mkNode(d_kind, sum, nm->mkConst<Rational>(0));

  // Normalize the constraint
  Node normalized = theory::Rewriter::rewrite(node);

  // Is the normalized constraint negated 
  bool negated = normalized.getKind() == kind::NOT;
  // Get the atom of the literal
  TNode atom = negated ? normalized[0] : normalized;
  // Construct the variable (this spawns notifications)
  Variable variable = db->getVariable(atom);

  // Return the literal
  Literal result(variable, negated);

  Debug("mcsat::linear") << "LinearConstraint::getLiteral(): " << result << std::endl;
  return result;
}


Variable LinearConstraint::getTopVariable(const SolverTrail& trail) const {
  Variable var = Variable::null;
  unsigned varLevel = 0;
  const_iterator it = begin();
  const_iterator it_end = end();
  for(; it != it_end; ++ it) {
    if (!it->first.isNull()) {
      Variable current = it->first;
      Assert(trail.hasValue(current));
      unsigned currentLevel = trail.decisionLevel(current);
      if (currentLevel > varLevel) {
	var = current;
	varLevel =  currentLevel;
      }
    }
  }
  Assert(var.isNull() || varLevel > 0);
  return var;  
}

Variable LinearConstraint::getUnassignedVariable(const SolverTrail& trail) const {
  const_iterator it = begin();
  const_iterator it_end = end();
  for(; it != it_end; ++ it) {    
    if (!it->first.isNull() && !trail.hasValue(it->first)) {
      return it->first;
    } 
  }
  return Variable::null;  
}