(define (quicksort lst)
  (if (null? lst)
      '()
      (let ((pivot (car lst))
            (rest (cdr lst)))
        (append
          (quicksort (filter (lambda (x) (< x pivot)) rest))
          (list pivot)
          (quicksort (filter (lambda (x) (>= x pivot)) rest))))))

(define data '(12 71 2 15 29 82 87 8 18 66 81 25 63 97 40 3 93 58 53 31 47))
(display (quicksort data))
(newline)