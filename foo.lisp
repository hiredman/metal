(set! y 5)
(plus x y)
((lambda (x y)
   (lambda ()
     (set! x 5)
   x))
 1 2)
