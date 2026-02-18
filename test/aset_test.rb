# Array/Hash element assignment tests
# Note: These tests verify []= operation, which is currently implemented using OP_SETIDX.
# OP_ASET (BBB: R[b][c] = R[a]) is defined in mrc_ops.h but not used in practice.
# Both the reference implementation (mruby-compiler) and mruby-compiler2 use OP_SETIDX instead,
# which works with consecutive registers: OP_SETIDX (B: R[a][R[a+1]] = R[a+2])

class AsetTest < Picotest::Test
  def test_array_simple_aset
    script = <<~RUBY
      a = [1, 2, 3]
      a[0] = 10
      a[1] = 20
      a[2] = 30
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[10, 20, 30]", actual)
  end

  def test_array_aset_with_variable_index
    script = <<~RUBY
      a = [1, 2, 3, 4, 5]
      i = 2
      a[i] = 99
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 99, 4, 5]", actual)
  end

  def test_array_aset_negative_index
    script = <<~RUBY
      a = [1, 2, 3]
      a[-1] = 100
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 100]", actual)
  end

  def test_hash_simple_aset
    script = <<~RUBY
      h = {}
      h[:a] = 1
      h[:b] = 2
      h[:c] = 3
      p h
    RUBY
    actual = run_script(script)
    assert_equal("{a: 1, b: 2, c: 3}", actual)
  end

  def test_hash_aset_with_string_key
    script = <<~RUBY
      h = {}
      h["key"] = "value"
      p h
    RUBY
    actual = run_script(script)
    assert_equal("{\"key\" => \"value\"}", actual)
  end

  def test_hash_aset_overwrite
    script = <<~RUBY
      h = {a: 1, b: 2}
      h[:a] = 100
      p h
    RUBY
    actual = run_script(script)
    assert_equal("{a: 100, b: 2}", actual)
  end

  def test_array_aset_in_expression
    script = <<~RUBY
      a = [1, 2, 3]
      result = (a[1] = 20)
      p result
      p a
    RUBY
    actual = run_script(script)
    assert_equal("20\n[1, 20, 3]", actual)
  end

  def test_nested_aset
    script = <<~RUBY
      a = [[1, 2], [3, 4]]
      a[0][1] = 99
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[[1, 99], [3, 4]]", actual)
  end

  def test_array_aset_with_computed_value
    script = <<~RUBY
      a = [1, 2, 3]
      a[1] = a[0] + a[2]
      p a
    RUBY
    actual = run_script(script)
    assert_equal("[1, 4, 3]", actual)
  end
end
