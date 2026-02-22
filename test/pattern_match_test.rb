class PatternMatchTest < Picotest::Test
  def test_value_pattern_with_integer
    script = <<~RUBY
      result = case 1
      in 1
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_value_pattern_with_string
    script = <<~RUBY
      result = case "hello"
      in "hello"
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_value_pattern_with_symbol
    script = <<~RUBY
      result = case :foo
      in :foo
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_value_pattern_with_true
    script = <<~RUBY
      result = case true
      in true
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_value_pattern_with_false
    script = <<~RUBY
      result = case false
      in false
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_value_pattern_with_nil
    script = <<~RUBY
      result = case nil
      in nil
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_variable_pattern
    script = <<~RUBY
      result = case 42
      in x
        x
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("42", actual)
  end

  def test_multiple_in_clauses
    script = <<~RUBY
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
    actual = run_script(script)
    assert_equal(":two", actual)
  end

  def test_else_clause
    script = <<~RUBY
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
    actual = run_script(script)
    assert_equal(":other", actual)
  end

  def test_no_match_returns_nil
    script = <<~RUBY
      result = case 99
      in 1
        :one
      in 2
        :two
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_alternative_pattern
    script = <<~RUBY
      result = case 2
      in 1 | 2 | 3
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_array_pattern_simple
    script = <<~RUBY
      result = case [1, 2, 3]
      in [1, 2, 3]
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_array_pattern_with_variables
    script = <<~RUBY
      result = case [1, 2, 3]
      in [a, b, c]
        [a, b, c]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_array_pattern_exact_length
    script = <<~RUBY
      result = case [1, 2]
      in [1, 2, 3]
        :no_match
      in [1, 2]
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_array_pattern_2_elements
    script = <<~RUBY
      result = case [1, 2]
      in [a, b]
        [a, b]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2]", actual)
  end

  def test_array_pattern_with_wildcard
    script = <<~RUBY
      result = case [1, 2, 3]
      in [_, x, _]
        x
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("2", actual)
  end

  def test_guard_clause_if
    script = <<~RUBY
      result = case 10
      in x if x > 5
        :big
      in x
        :small
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":big", actual)
  end

  def test_guard_clause_unless
    script = <<~RUBY
      result = case 3
      in x unless x > 5
        :small
      in x
        :big
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":small", actual)
  end

  def test_guard_clause_with_array_pattern
    script = <<~RUBY
      result = case [1, 2, 3]
      in [a, b, c] if a + b + c > 5
        :sum_big
      in [a, b, c]
        :sum_small
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":sum_big", actual)
  end

  def test_guard_clause_failing
    script = <<~RUBY
      result = case 10
      in x if x > 20
        :first
      in x
        :second
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":second", actual)
  end

  def test_pin_operator_basic
    script = <<~RUBY
      x = 1
      result = case 1
      in ^x
        :matched
      else
        :not_matched
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":matched", actual)
  end

  def test_pin_operator_no_match
    script = <<~RUBY
      a = 1
      result = case 2
      in ^a
        :same
      else
        :different
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":different", actual)
  end

  def test_pin_operator_in_array
    script = <<~RUBY
      expected = 42
      result = case [42, 100]
      in [^expected, y]
        y
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("100", actual)
  end

  def test_alternative_in_array_pattern
    script = <<~RUBY
      result = case [3, 4]
      in [1, 2] | [3, 4]
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_array_pattern_with_literal_and_variable
    script = <<~RUBY
      result = case [1, 2, 3]
      in [1, 2, x]
        x
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("3", actual)
  end

  def test_empty_array_pattern
    script = <<~RUBY
      result = case []
      in []
        :empty
      else
        :not_empty
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":empty", actual)
  end

  def test_alternative_pattern_first_branch
    script = <<~RUBY
      result = case 1
      in 1 | 2 | 3
        :found
      else
        :not_found
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":found", actual)
  end

  def test_alternative_pattern_third_branch
    script = <<~RUBY
      result = case 3
      in 1 | 2 | 3
        :found
      else
        :not_found
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":found", actual)
  end

  def test_alternative_pattern_no_match
    script = <<~RUBY
      result = case 5
      in 1 | 2 | 3
        :found
      else
        :not_found
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":not_found", actual)
  end

  def test_deconstruct_returns_nil
    script = <<~RUBY
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
    actual = run_script(script)
    assert_equal(":no_match", actual)
  end

  def test_deconstruct_keys_returns_nil
    script = <<~RUBY
      class Foo
        def deconstruct_keys(keys)
          nil
        end
      end
      result = case Foo.new
      in {a: 1}
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":no_match", actual)
  end

  def test_deconstruct_returns_nil_for_find_pattern
    script = <<~RUBY
      class Foo
        def deconstruct
          nil
        end
      end
      result = case Foo.new
      in [*, 1, 2, *]
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":no_match", actual)
  end

  def test_hash_pattern_with_nil_matches_exact_keys
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1}
      in {a: 1, **nil}
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_hash_pattern_with_nil_rejects_extra_keys
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2}
      in {a: 1, **nil}
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":no_match", actual)
  end

  def test_hash_pattern_with_missing_key_does_not_match
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {b: 1}
      in {a: nil}
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":no_match", actual)
  end

  def test_hash_pattern_with_nil_value_matches
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: nil}
      in {a: nil}
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_case_with_splat_in_array_literal_predicate
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      arr = [2, 3]
      result = case [1, *arr, 4]
      in [1, 2, 3, 4]
        :match
      else
        :no_match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_array_pattern_with_rest
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [first, *rest, last]
        [first, rest, last]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, [2, 3, 4], 5]", actual)
  end

  def test_array_pattern_nested_simple
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [[1, 2]]
      in [[a, b]]
        [a, b]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2]", actual)
  end

  def test_array_pattern_nested
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [[1, 2], [3, 4]]
      in [[a, b], [c, d]]
        [a, b, c, d]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3, 4]", actual)
  end

  def test_hash_pattern_simple
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2}
      in {a: 1, b: 2}
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_hash_pattern_with_variables
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2}
      in {a: x, b: y}
        [x, y]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2]", actual)
  end

  def test_hash_pattern_shorthand
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2}
      in {a:, b:}
        [a, b]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2]", actual)
  end

  def test_hash_pattern_partial_match
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2, c: 3}
      in {a: x}
        x
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_hash_pattern_nested
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: {x: 1}, b: {y: 2}}
      in {a: {x: n}, b: {y: m}}
        [n, m]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2]", actual)
  end

  def test_find_pattern_basic
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [*, 2, 3, *]
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal(":match", actual)
  end

  def test_find_pattern_with_capture
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [*pre, 2, 3, *post]
        [pre, post]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[[1], [4, 5]]", actual)
  end

  def test_find_pattern_no_match
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3]
      in [*, 5, 6, *]
        :match
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_find_pattern_at_beginning
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3]
      in [*pre, 1, *post]
        [pre, post]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[[], [2, 3]]", actual)
  end

  def test_find_pattern_at_end
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3]
      in [*pre, 3, *post]
        [pre, post]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[[1, 2], []]", actual)
  end

  def test_find_pattern_multiple_elements
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [*pre, 2, x, *post]
        [pre[0], x, post]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 3, [4, 5]]", actual)
  end

  def test_as_pattern_with_array
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3]
      in [x, *rest] => whole
        [x, rest, whole]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, [2, 3], [1, 2, 3]]", actual)
  end

  def test_as_pattern_with_hash
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {a: 1, b: 2}
      in {a:} => h
        [a, h]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, {a: 1, b: 2}]", actual)
  end

  def test_array_pattern_with_first_element
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [first, *rest]
        [first, rest]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, [2, 3, 4, 5]]", actual)
  end

  def test_deeply_nested_array_and_hash
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case {outer: [{inner: [1, 2]}, 3]}
      in {outer: [{inner: [a, b]}, c]}
        [a, b, c]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_complex_pattern_with_all_features
    skip "Waiting for merging mrubyc#257" if mrubyc?
    script = <<~RUBY
      result = case [1, 2, 3, 4, 5]
      in [1, *rest] => arr if arr.size > 2
        [rest[0], rest[1..1], rest[2..-1]]
      end
      p result
    RUBY
    actual = run_script(script)
    assert_equal("[2, [3], [4, 5]]", actual)
  end

end
