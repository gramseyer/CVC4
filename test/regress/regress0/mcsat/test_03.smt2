(set-logic QF_LRA)
(set-info :smt-lib-version 2.0)
(set-info :status sat)
(declare-fun x0 () Bool)
(declare-fun x1 () Bool)
(declare-fun x2 () Bool)
(assert (not x0))
(assert (not x1))
(assert (or x0 x1 x2))
(check-sat)
(exit)
