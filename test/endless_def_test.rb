class EndlessDefTest < Picotest::Test
  def test_endless_def_arg
    script = <<~RUBY
      def fib(n) = n < 2 ? n : fib(n-1) + fib(n-2)
      p fib(10)
    RUBY
    actual = run_script(script)
    assert_equal("55", actual)
  end

  def test_endless_def_command
    script = <<~RUBY
      def hey(word) = puts word
      hey "you!"
    RUBY
    actual = run_script(script)
    assert_equal("you!", actual)
  end
end
