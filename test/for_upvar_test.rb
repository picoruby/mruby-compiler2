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
end
