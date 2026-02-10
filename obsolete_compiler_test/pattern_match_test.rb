class PatternMatchTest < PicoRubyTest
  desc "value pattern with integer"
  assert_equal(<<~RUBY, ":match")
    result = case 1
    in 1
      :match
    end
    p result
  RUBY

  desc "value pattern with string"
  assert_equal(<<~RUBY, ":match")
    result = case "hello"
    in "hello"
      :match
    end
    p result
  RUBY

  desc "value pattern with symbol"
  assert_equal(<<~RUBY, ":match")
    result = case :foo
    in :foo
      :match
    end
    p result
  RUBY

  desc "value pattern with true"
  assert_equal(<<~RUBY, ":match")
    result = case true
    in true
      :match
    end
    p result
  RUBY

  desc "value pattern with false"
  assert_equal(<<~RUBY, ":match")
    result = case false
    in false
      :match
    end
    p result
  RUBY

  desc "value pattern with nil"
  assert_equal(<<~RUBY, ":match")
    result = case nil
    in nil
      :match
    end
    p result
  RUBY

  desc "variable pattern"
  assert_equal(<<~RUBY, "42")
    result = case 42
    in x
      x
    end
    p result
  RUBY

  desc "multiple in clauses"
  assert_equal(<<~RUBY, ":two")
    result = case 2
    in 1
      :one
    in 2
      :two
    in 3
      :three
    end
    p result
  RUBY

  desc "else clause"
  assert_equal(<<~RUBY, ":other")
    result = case 99
    in 1
      :one
    in 2
      :two
    else
      :other
    end
    p result
  RUBY

  desc "no match returns nil"
  assert_equal(<<~RUBY, "nil")
    result = case 99
    in 1
      :one
    in 2
      :two
    end
    p result
  RUBY

  desc "alternative pattern"
  assert_equal(<<~RUBY, ":match")
    result = case 2
    in 1 | 2 | 3
      :match
    end
    p result
  RUBY

  desc "array pattern simple"
  assert_equal(<<~RUBY, ":match")
    result = case [1, 2, 3]
    in [1, 2, 3]
      :match
    end
    p result
  RUBY

  desc "array pattern with variables"
  assert_equal(<<~RUBY, "[1, 2, 3]")
    result = case [1, 2, 3]
    in [a, b, c]
      [a, b, c]
    end
    p result
  RUBY

  desc "array pattern exact length"
  assert_equal(<<~RUBY, ":match")
    result = case [1, 2]
    in [1, 2, 3]
      :no_match
    in [1, 2]
      :match
    end
    p result
  RUBY

  desc "array pattern 2 elements"
  assert_equal(<<~RUBY, "[1, 2]")
    result = case [1, 2]
    in [a, b]
      [a, b]
    end
    p result
  RUBY

  desc "array pattern with wildcard"
  assert_equal(<<~RUBY, "2")
    result = case [1, 2, 3]
    in [_, x, _]
      x
    end
    p result
  RUBY

  desc "guard clause if"
  assert_equal(<<~RUBY, ":big")
    result = case 10
    in x if x > 5
      :big
    in x
      :small
    end
    p result
  RUBY

  desc "guard clause unless"
  assert_equal(<<~RUBY, ":small")
    result = case 3
    in x unless x > 5
      :small
    in x
      :big
    end
    p result
  RUBY

  desc "guard clause with array pattern"
  assert_equal(<<~RUBY, ":sum_big")
    result = case [1, 2, 3]
    in [a, b, c] if a + b + c > 5
      :sum_big
    in [a, b, c]
      :sum_small
    end
    p result
  RUBY

  desc "guard clause failing"
  assert_equal(<<~RUBY, ":second")
    result = case 10
    in x if x > 20
      :first
    in x
      :second
    end
    p result
  RUBY

  desc "pin operator basic"
  assert_equal(<<~RUBY, ":matched")
    x = 1
    result = case 1
    in ^x
      :matched
    else
      :not_matched
    end
    p result
  RUBY

  desc "pin operator no match"
  assert_equal(<<~RUBY, ":different")
    a = 1
    result = case 2
    in ^a
      :same
    else
      :different
    end
    p result
  RUBY

  desc "pin operator in array"
  assert_equal(<<~RUBY, "100")
    expected = 42
    result = case [42, 100]
    in [^expected, y]
      y
    end
    p result
  RUBY

  desc "alternative in array pattern"
  assert_equal(<<~RUBY, ":match")
    result = case [3, 4]
    in [1, 2] | [3, 4]
      :match
    else
      :no_match
    end
    p result
  RUBY

  desc "array pattern with literal and variable"
  assert_equal(<<~RUBY, "3")
    result = case [1, 2, 3]
    in [1, 2, x]
      x
    end
    p result
  RUBY

  desc "empty array pattern"
  assert_equal(<<~RUBY, ":empty")
    result = case []
    in []
      :empty
    else
      :not_empty
    end
    p result
  RUBY

  desc "alternative pattern first branch"
  assert_equal(<<~RUBY, ":found")
    result = case 1
    in 1 | 2 | 3
      :found
    else
      :not_found
    end
    p result
  RUBY

  desc "alternative pattern third branch"
  assert_equal(<<~RUBY, ":found")
    result = case 3
    in 1 | 2 | 3
      :found
    else
      :not_found
    end
    p result
  RUBY

  desc "alternative pattern no match"
  assert_equal(<<~RUBY, ":not_found")
    result = case 5
    in 1 | 2 | 3
      :found
    else
      :not_found
    end
    p result
  RUBY

  desc "case with splat in array literal predicate"
  assert_equal(<<~RUBY, ":match")
    arr = [2, 3]
    result = case [1, *arr, 4]
    in [1, 2, 3, 4]
      :match
    else
      :no_match
    end
    p result
  RUBY

  desc "deconstruct returns nil"
  assert_equal(<<~RUBY, ":no_match")
    class Foo
      def deconstruct
        nil
      end
    end
    result = case Foo.new
    in [a, b]
      :match
    else
      :no_match
    end
    p result
  RUBY

  desc "hash pattern with nil value matches"
  assert_equal(<<~RUBY, ":match")
    result = case {a: nil}
    in {a: nil}
      :match
    else
      :no_match
    end
    p result
  RUBY

  desc "hash pattern with missing key does not match"
  assert_equal(<<~RUBY, ":no_match")
    result = case {b: 1}
    in {a: nil}
      :match
    else
      :no_match
    end
    p result
  RUBY

  # TODO
  # The rest of the pattern matching features needs merging PR:
  #   https://github.com/mrubyc/mrubyc/pull/257
  pending unless ENV['USE_MRUBY']

  desc "array pattern with rest"
  assert_equal(<<~RUBY, "[1, [2, 3, 4], 5]")
    result = case [1, 2, 3, 4, 5]
    in [first, *rest, last]
      [first, rest, last]
    end
    p result
  RUBY

  desc "array pattern nested simple"
  assert_equal(<<~RUBY, "[1, 2]")
    result = case [[1, 2]]
    in [[a, b]]
      [a, b]
    end
    p result
  RUBY

  desc "array pattern nested"
  assert_equal(<<~RUBY, "[1, 2, 3, 4]")
    result = case [[1, 2], [3, 4]]
    in [[a, b], [c, d]]
      [a, b, c, d]
    end
    p result
  RUBY

  desc "hash pattern simple"
  assert_equal(<<~RUBY, ":match")
    result = case {a: 1, b: 2}
    in {a: 1, b: 2}
      :match
    end
    p result
  RUBY

  desc "hash pattern with variables"
  assert_equal(<<~RUBY, "[1, 2]")
    result = case {a: 1, b: 2}
    in {a: x, b: y}
      [x, y]
    end
    p result
  RUBY

  desc "hash pattern shorthand"
  assert_equal(<<~RUBY, "[1, 2]")
    result = case {a: 1, b: 2}
    in {a:, b:}
      [a, b]
    end
    p result
  RUBY

  desc "hash pattern partial match"
  assert_equal(<<~RUBY, "1")
    result = case {a: 1, b: 2, c: 3}
    in {a: x}
      x
    end
    p result
  RUBY

  desc "hash pattern nested"
  assert_equal(<<~RUBY, "[1, 2]")
    result = case {a: {x: 1}, b: {y: 2}}
    in {a: {x: n}, b: {y: m}}
      [n, m]
    end
    p result
  RUBY

  desc "find pattern basic"
  assert_equal(<<~RUBY, ":match")
    result = case [1, 2, 3, 4, 5]
    in [*, 2, 3, *]
      :match
    end
    p result
  RUBY

  desc "find pattern with capture"
  assert_equal(<<~RUBY, "[[1], [4, 5]]")
    result = case [1, 2, 3, 4, 5]
    in [*pre, 2, 3, *post]
      [pre, post]
    end
    p result
  RUBY

  desc "find pattern no match"
  assert_equal(<<~RUBY, "nil")
    result = case [1, 2, 3]
    in [*, 5, 6, *]
      :match
    end
    p result
  RUBY

  desc "find pattern at beginning"
  assert_equal(<<~RUBY, "[[], [2, 3]]")
    result = case [1, 2, 3]
    in [*pre, 1, *post]
      [pre, post]
    end
    p result
  RUBY

  desc "find pattern at end"
  assert_equal(<<~RUBY, "[[1, 2], []]")
    result = case [1, 2, 3]
    in [*pre, 3, *post]
      [pre, post]
    end
    p result
  RUBY

  desc "find pattern multiple elements"
  assert_equal(<<~RUBY, "[1, 3, [4, 5]]")
    result = case [1, 2, 3, 4, 5]
    in [*pre, 2, x, *post]
      [pre[0], x, post]
    end
    p result
  RUBY
  desc "as pattern with array"
  assert_equal(<<~RUBY, "[1, [2, 3], [1, 2, 3]]")
    result = case [1, 2, 3]
    in [x, *rest] => whole
      [x, rest, whole]
    end
    p result
  RUBY

  desc "as pattern with hash"
  assert_equal(<<~RUBY, "[1, {a: 1, b: 2}]")
    result = case {a: 1, b: 2}
    in {a:} => h
      [a, h]
    end
    p result
  RUBY

  desc "array pattern with first element"
  assert_equal(<<~RUBY, "[1, [2, 3, 4, 5]]")
    result = case [1, 2, 3, 4, 5]
    in [first, *rest]
      [first, rest]
    end
    p result
  RUBY

  desc "deeply nested array and hash"
  assert_equal(<<~RUBY, "[1, 2, 3]")
    result = case {outer: [{inner: [1, 2]}, 3]}
    in {outer: [{inner: [a, b]}, c]}
      [a, b, c]
    end
    p result
  RUBY

  desc "complex pattern with all features"
  assert_equal(<<~RUBY, "[2, [3], [4, 5]]")
    result = case [1, 2, 3, 4, 5]
    in [1, *rest] => arr if arr.size > 2
      [rest[0], rest[1..1], rest[2..-1]]
    end
    p result
  RUBY
end
