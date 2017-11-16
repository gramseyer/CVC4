(set-logic QF_LRA)
(set-info :smt-lib-version 2.0)
(declare-fun x0 () Real)
(declare-fun x1 () Real)
(assert (> (+ (* 1 x0) (- 0)) 0))
(assert (not (= (+ (* (- 10) x1) (* 1 x0) (- 1)) 0)))
(check-sat)
(exit)