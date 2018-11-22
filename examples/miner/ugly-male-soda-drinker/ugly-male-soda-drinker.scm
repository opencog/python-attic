;; Load the miner module
(use-modules (opencog miner))

;; For debugging
(use-modules (opencog logger))
(use-modules (opencog rule-engine))
(ure-logger-set-level! "debug")

;; Load KB
(load "kb.scm")

;; Call the miner on the entire atomspace with minimum support of 5,
;; 100 iterations of forward chaining, a 0.1 strength and confidence
;; of expanding conjunctions up at most 3 conjuncts.
;;
;; Expect to learn, amonst others, the following pattern
;;
;; (Lambda
;;   (Variable "$X")
;;   (And
;;     (Inheritance
;;       (Variable "$X")
;;       (Concept "man"))
;;     (Inheritance
;;       (Variable "$X")
;;       (Concept "ugly"))
;;     (Inheritance
;;       (Variable "$X")
;;       (Concept "soda drinker"))))
(cog-mine (cog-atomspace) 5
          #:maxiter 100
          #:incremental-expansion (stv 0.1 0.1)
          #:max-conjuncts 3)
