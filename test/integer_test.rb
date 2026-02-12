class IntegerTest < Picotest::Test
  def test_binary_literal_8bit
    script = <<~RUBY
      p(0b11111111)
    RUBY
    actual = run_script(script)
    assert_equal("255", actual)
  end

  def test_binary_literal_16bit
    script = <<~RUBY
      p(0b1111111111111111)
    RUBY
    actual = run_script(script)
    assert_equal("65535", actual)
  end

  def test_binary_literal_17bit
    script = <<~RUBY
      p(0b11111111111111111)
    RUBY
    actual = run_script(script)
    assert_equal("131071", actual)
  end

  def test_binary_literal_negative
    script = <<~RUBY
      p(-0b11111111111111111)
    RUBY
    actual = run_script(script)
    assert_equal("-131071", actual)
  end

  def test_octal_literal
    script = <<~RUBY
      p(0o7777777)
    RUBY
    actual = run_script(script)
    assert_equal("2097151", actual)
  end

  def test_octal_literal_negative
    script = <<~RUBY
      p(-0o7777777)
    RUBY
    actual = run_script(script)
    assert_equal("-2097151", actual)
  end

  def test_hexadecimal_literal_16bit
    script = <<~RUBY
      p(0xFFFF)
    RUBY
    actual = run_script(script)
    assert_equal("65535", actual)
  end

  def test_hexadecimal_literal_20bit
    script = <<~RUBY
      p(0xFFFFF)
    RUBY
    actual = run_script(script)
    assert_equal("1048575", actual)
  end

  def test_hexadecimal_literal_negative
    script = <<~RUBY
      p(-0xFFFFF)
    RUBY
    actual = run_script(script)
    assert_equal("-1048575", actual)
  end

  def test_abs_method
    script = <<~RUBY
      p(-70000.abs)
    RUBY
    actual = run_script(script)
    assert_equal("70000", actual)
  end

  def test_to_i_method_positive_float
    script = <<~RUBY
      p(7000.001.to_i)
    RUBY
    actual = run_script(script)
    assert_equal("7000", actual)
  end

  def test_to_i_method_negative_float
    script = <<~RUBY
      p(-7000.001.to_i)
    RUBY
    actual = run_script(script)
    assert_equal("-7000", actual)
  end

  def test_0755_is_eq_to_0o755
    script = <<~RUBY
      p 0755
    RUBY
    actual = run_script(script)
    assert_equal("493", actual)
  end
end
