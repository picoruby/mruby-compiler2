class OpAssignTest < Picotest::Test
  def test_op_assign_with_both_array_set_and_attr_set
    script = <<~RUBY
      class Array
        def first
          self[0]
        end
        def first=(val)
          self[0] = val
        end
      end
      ary = [1, 2]
      ary.first += 3
      ary[1] *= 3
      p ary
    RUBY
    actual = run_script(script)
    assert_equal("[4, 6]", actual)
  end

  def test_left_shift_assign
    script = <<~RUBY
      a = 2
      a <<= 3
      p a
    RUBY
    actual = run_script(script)
    assert_equal("16", actual)
  end

  def test_return_value
    script = <<~RUBY
      a = {}
      p(a[:a] ||= 1)
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_local_var_add_assign
    script = <<~RUBY
      i = 0
      i += 1
      i += 1
      i += 1
      p i
    RUBY
    actual = run_script(script)
    assert_equal("3", actual)
  end

  def test_local_var_sub_assign
    script = <<~RUBY
      i = 10
      i -= 3
      i -= 2
      p i
    RUBY
    actual = run_script(script)
    assert_equal("5", actual)
  end

  def test_local_var_add_assign_in_loop
    script = <<~RUBY
      sum = 0
      i = 0
      while i < 5
        sum += i
        i += 1
      end
      p sum
    RUBY
    actual = run_script(script)
    assert_equal("10", actual)
  end
end
