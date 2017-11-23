(define (println text)
  (display text)
  (newline))

(define do-hello
  (lambda ()
    (display "hello world")
    (newline)))

(println "hello.scm is loaded ...")

(when #f 
  (println "version info ...")
  (edi-show-version))

