(var (sum x) (if (= x 100)
                 0
                 (+ x (sum (+ x 1))))
     (sum 1))
