Feval - evaluation using f-algebras
===================================

Author: Anthony Burzillo

******

cfl (standing for C-based Functional Language) is a statically typed
functional programming language.

## Usage

To build run `make`, which builds the interpreter `cfl`. To simply
run the interpreter on a file run
```
./cfl filename
```
To output the internal AST of a file run
```
./cfl -ast filename
```
To output the return type of a file run
```
./cfl -type filename
```

## Example

The mergesort algorithm can be written in cfl as
```
compare x y = if x > y then -1 else if x == y then 0 else 1;

combine x y = case x of
      []       -> y
    | (z : zs) -> case y of
          []       -> x
        | (w : ws) -> let r = compare z w in if r <= 0
            then w : z : combine zs ws
            else z : w : combine zs ws; 

mergesort x = let mergesort' x l r = case x of
      []       -> combine (mergesort l) (mergesort r)
    | (z : zs) -> case zs of
          []       -> x
        | (w : ws) -> mergesort' ws (z : l) (w : r)
in mergesort' x [] []; 

main = mergesort [4, 23, -34, 3]
```
Note that every cfl program must end with a `main` and global defintions are followed by
a semicolon.

## Expressions

### Boolean Operations

We allow conjunction (&&), disjunction (||), and negation (!) expressions.

### Integer Operations

The operations of addition (+), subtraction (-), multiplication (*), division (/),
and modulus (%) evaluate to integer values. On the other hand, the comparison
operators of equality (==), less-than (<), less-than-or-equal (<=), greater-than
(>), and greater-than-or-equal (>=) all evaluate to boolean values.

### Functional Operations

We allow the creation of anonymous functions via `function x -> e` where `e` is some
expression, and `x` is the argument to the function. We can create multiple argument
anonymous functions via `function x -> function y -> e` where `x` is
the first argument and `y` is the second, etc.

To apply a function `f` simply use `f e` where `e` is the expression for the first
argument, or `f e1 e2` for the first argument `e1` and second argument `e2`.

### If Expressions

Use `if e1 then e2 else e3`.

### Let Expressions

We allow let expressions to define constants and functions (possibly recursive) via
```
let x = 4 in x + 54
```
and
```
let f x y = if x = 0 then 0 else y + f (x - 1) y in f 3 4
```

### List Expressions

You can create empty lists `[]` or lists with values `[1, 2, 3, 4]`. You can push
values onto a list via
```
5 : [4, 5, 6]
  => [5, 4, 5, 6] 
```
You can also concatenate lists
```
[1, 2] ++ [3, 4]
  => [1, 2, 3, 4]
```
Finally, we can match on lists via a case expression via
```
case e1 of [] -> e2 | (x : xs) -> e3
```
