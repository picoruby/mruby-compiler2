class StringTest < Picotest::Test
  def test_keyword_in_str
    script = <<~RUBY
      puts "nil"
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_percent_q
    script = <<~'RUBY'
      puts %q!a\!b!
    RUBY
    actual = run_script(script)
    assert_equal("a!b", actual)
  end

  def test_percent_q_capital
    script = <<~'RUBY'
      puts %Q!a\!#{3 ** 2}b!
    RUBY
    actual = run_script(script)
    assert_equal("a!9b", actual)
  end

  def test_null_letter_strip
    script = <<~'RUBY'
      s = "\0"
      puts s.strip!
    RUBY
    actual = run_script(script)
    assert_equal("", actual)
  end

  def test_getter_x_y
    script = <<~'RUBY'
      s = "bar"
      puts s[2, 0]
    RUBY
    actual = run_script(script)
    assert_equal("", actual)
  end

  def test_binary
    script = <<~'RUBY'
      p "\x00".size
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end
end
