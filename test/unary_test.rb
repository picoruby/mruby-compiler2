class UnaryTest < Picotest::Test
  def test_p_minus_1
    script = <<~RUBY
      p -1
    RUBY
    actual = run_script(script)
    assert_equal("-1", actual)
  end

  def test_after_pow
    script = <<~RUBY
      p 10**-2
    RUBY
    actual = run_script(script)
    expected = mruby? ? "0.01" : "0"
    assert_equal(expected, actual)
  end

  def test_minus_1_pow_2
    script = <<~RUBY
      p -1**2
    RUBY
    actual = run_script(script)
    assert_equal("-1", actual)
  end

  def test_minus_1_3_pow_3
    script = <<~RUBY
      p -1.3**3
    RUBY
    actual = run_script(script)
    assert_equal("-2.197", actual)
  end
end
