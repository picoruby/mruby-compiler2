class FloatTest < Picotest::Test
  def test_plus
    script = <<~RUBY
      p 1.1
    RUBY
    actual = run_script(script)
    assert_equal("1.1", actual)
  end

  def test_minus
    script = <<~RUBY
      p -1.1
    RUBY
    actual = run_script(script)
    assert_equal("-1.1", actual)
  end
end
