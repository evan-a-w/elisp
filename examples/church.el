(let ((zero f x) (f x)
      (succ n f x) (f (n f x))
      (plus m n f x) (m f (n f x))
      (times m n f x) (m (n f) x)
      (two) (succ (succ zero)))
(plus (times two two) (succ two)))
