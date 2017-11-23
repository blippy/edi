;;;; attempt at a pure implementation of ed
;;;; 2017-11-23 mcarter created (also, project creation date)


(use-modules (srfi srfi-1) (srfi srfi-11))

(define (println x)
  (display x)
  (newline))

;;(define lines (make-vector 0))
(define lines '())
(define active-line 0)

(define (insert line)
  ;;(println active-line)
  (let-values (((before after) (split-at lines active-line)))
	      (set! lines (append before (cons line after)))
	      (set! active-line (1+ active-line))
	      #t)	
  ;;(println (split-at lines active-line))
  #t)


(define (up-line)
  (set! active-line (1- active-line)))

(insert "foo")
(up-line)
(insert "this should be at the top")
(insert "bar")
(insert "how now brown cow")
(up-line)
(insert "quicky insert")
;;(display lines)

(for-each println lines)

