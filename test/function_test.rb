class FunctionTest < Picotest::Test
  def test_one_arg_without_paren
    script = <<~RUBY
      puts "Hello"
    RUBY
    actual = run_script(script)
    assert_equal("Hello", actual)
  end

  def test_one_arg_with_paren
    script = <<~RUBY
      puts("Hello")
    RUBY
    actual = run_script(script)
    assert_equal("Hello", actual)
  end

  def test_multiple_args_without_paren
    script = <<~RUBY
      puts "String", :symbol, 1, 3.14
    RUBY
    actual = run_script(script)
    assert_equal("String\nsymbol\n1\n3.14", actual)
  end

  def test_multiple_args_with_paren
    script = <<~RUBY
      puts("String", :symbol, 1, 3.14)
    RUBY
    actual = run_script(script)
    assert_equal("String\nsymbol\n1\n3.14", actual)
  end

  def test_p_p_p_0
    script = <<~RUBY
      p p p 0
    RUBY
    actual = run_script(script)
    assert_equal("0\n0\n0", actual)
  end

  def test_p_p_p
    script = <<~RUBY
      p p p
    RUBY
    actual = run_script(script)
    assert_equal("nil\nnil", actual)
  end
end
