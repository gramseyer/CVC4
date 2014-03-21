; EXPECT: sat
(set-logic QF_ALL_SUPPORTED)
(set-info :status sat)
(define-sort Elt () Int)
(define-sort mySet () (Set Elt ))
(define-fun smt_set_emp () mySet (as emptyset mySet))
(define-fun smt_set_mem ((x Elt) (s mySet)) Bool (in x s))
(define-fun smt_set_add ((s mySet) (x Elt)) mySet (union s (setenum x)))
(define-fun smt_set_cup ((s1 mySet) (s2 mySet)) mySet (union s1 s2))
(define-fun smt_set_cap ((s1 mySet) (s2 mySet)) mySet (intersection s1 s2))
;(define-fun smt_set_com ((s mySet)) mySet ((_ map not) s))
(define-fun smt_set_dif ((s1 mySet) (s2 mySet)) mySet (setminus s1 s2))
;(define-fun smt_set_sub ((s1 mySet) (s2 mySet)) Bool (= smt_set_emp (smt_set_dif s1 s2)))
(define-fun smt_set_sub ((s1 mySet) (s2 mySet)) Bool (subseteq s1 s2))
(declare-fun z3v58 () Int)
(declare-fun z3v59 () Int)
(assert (distinct z3v58 z3v59))
(declare-fun z3f60 (Int) Bool)
(declare-fun z3v61 () Int)
(declare-fun z3f62 (Int) Int)
(declare-fun z3v63 () Int)
(declare-fun z3v64 () Int)
(declare-fun z3v65 () Int)
(declare-fun z3v66 () Int)
(declare-fun z3f67 (Int) mySet)
(declare-fun z3v69 () Int)
(declare-fun z3f70 (Int) Int)
(declare-fun z3v76 () Int)
(declare-fun z3v77 () Int)
(declare-fun z3v78 () Int)
(declare-fun z3v79 () Int)
(declare-fun z3v80 () Int)
(declare-fun z3v81 () Int)
(declare-fun z3v82 () Int)
(declare-fun z3f83 (Int) Int)
(declare-fun z3f84 (Int) Int)
(declare-fun z3v85 () Int)
(declare-fun z3f86 (Int) Int)
(declare-fun z3f87 (Int Int) Int)
(declare-fun z3v88 () Int)
(declare-fun z3v89 () Int)
(declare-fun z3f90 (Int) mySet)
(declare-fun z3f91 (Int) Bool)
(declare-fun z3f92 (Int Int) Int)
(declare-fun z3v93 () Int)
(declare-fun z3v94 () Int)
(declare-fun z3v95 () Int)
(declare-fun z3v96 () Int)
(declare-fun z3v97 () Int)
(assert (and (not (z3f60 z3v79)) (not (z3f60 z3v79)) (= z3v79 z3v80) (= (z3f60 z3v79) (= z3v76 z3v81)) (= (z3f60 z3v80) (= z3v76 z3v81)) (= (z3f83 z3v82) z3v81) (= (z3f84 z3v82) z3v81) (= (z3f86 z3v82) z3v85) (= z3v82 (z3f87 z3v81 z3v85)) (= z3v82 z3v88) (>= (z3f70 z3v78) 0) (= (z3f67 z3v78) (smt_set_cup (smt_set_add smt_set_emp (z3f83 z3v88)) (z3f67 z3v89))) (= (z3f90 z3v78) (smt_set_cup (smt_set_add smt_set_emp z3v88) (z3f90 z3v89))) (= (z3f70 z3v78) (+ 1 (z3f70 z3v89))) (= (z3f91 z3v78) false) (= z3v78 (z3f92 z3v88 z3v89)) (>= (z3f70 z3v78) 0) (= z3v78 z3v77) (>= (z3f70 z3v78) 0) (>= (z3f70 z3v89) 0) (>= (z3f70 z3v77) 0) (>= (z3f70 z3v97) 0) (= z3v97 z3v89) (>= (z3f70 z3v97) 0) (z3f60 z3v58) (= (z3f62 z3v61) z3v61) (= (z3f62 z3v63) z3v63) (not (z3f60 z3v59)) (= (z3f62 z3v64) z3v64)))
(assert (smt_set_mem z3v76 (z3f67 z3v78)))
(assert (<= z3v95 z3v93))
(assert (>= z3v95 z3v93))
(assert (= z3v95 z3v93))
(assert (smt_set_mem z3v76 (z3f67 z3v77)))
(declare-fun z3v98 () Int)
(assert (not (<  z3v98 z3v85)))
(check-sat)
