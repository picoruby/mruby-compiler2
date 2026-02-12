class AndOrTest < Picotest::Test
  def test_and
    script = <<~RUBY
      p true && nil
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_or_1
    script = <<~RUBY
      puts nil || true
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_or_2
    script = <<~RUBY
      puts true || false
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end
end
