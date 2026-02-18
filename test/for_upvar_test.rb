class ForLoopUpvarTest < Picotest::Test
  def test_for_loop_with_outer_variable_reference
    script = <<~RUBY
      ans = []
      1.times{
        for n in 1..3
          ans << n
        end
      }
      p ans
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_for_loop_nested_in_blocks
    script = <<~RUBY
      ans = []
      1.times {
        for n in 1..3
          a = n
          ans << a
        end
      }
      p ans
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_for_loop_with_outer_variable_read_only
    script = <<~RUBY
      ans = []
      1.times{
        for n in 1..3
          p ans
        end
      }
    RUBY
    actual = run_script(script)
    assert_equal("[]\n[]\n[]", actual)
  end

  def test_block_inside_for_accesses_for_variable
    script = <<~RUBY
      for x in [1, 2, 3]
        [0].each { p x }
      end
    RUBY
    actual = run_script(script)
    assert_equal("1\n2\n3", actual)
  end

  def test_block_inside_nested_for_accesses_for_variables
    script = <<~RUBY
      for m in [1, 2]
        for n in [10, 20]
          [0].each { p [m, n] }
        end
      end
    RUBY
    actual = run_script(script)
    assert_equal("[1, 10]\n[1, 20]\n[2, 10]\n[2, 20]", actual)
  end

  def test_block_inside_for_accesses_outer_and_for_variables
    script = <<~RUBY
      base = 100
      for x in [1, 2]
        [0].each { p base + x }
      end
    RUBY
    actual = run_script(script)
    assert_equal("101\n102", actual)
  end
end
