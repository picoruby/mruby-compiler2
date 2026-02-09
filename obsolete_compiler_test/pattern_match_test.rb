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

  desc "array pattern with rest"
  assert_equal(<<~RUBY, "[1, [2, 3, 4], 5]")
    result = case [1, 2, 3, 4, 5]
    in [first, *rest, last]
      [first, rest, last]
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

  desc "array pattern with wildcard"
  assert_equal(<<~RUBY, "2")
    result = case [1, 2, 3]
    in [_, x, _]
      x
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
end
