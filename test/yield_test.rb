class YieldTest < Picotest::Test
  def test_do_block_1
    script = <<~'RUBY'
      def my_method(m)
        yield m
      end
      my_method("Ruby") do |v|
        puts "Hello #{v}"
      end
    RUBY
    actual = run_script(script)
    assert_equal("Hello Ruby", actual)
  end

  def test_do_block_2
    script = <<~'RUBY'
      def my_method(m, n)
       yield m, n
      end
      my_method("Ruby", "Pico") do |v, x|
        puts "Hello #{x}#{v}"
      end
    RUBY
    actual = run_script(script)
    assert_equal("Hello PicoRuby", actual)
  end

  def test_bang_yield_self
    script = <<~'RUBY'
      def my_method
        puts false  if ! yield self
      end
      my_method do
        false
      end
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end

  def test_yield_in_nested_irep
    script = <<~'RUBY'
      def exec(sql, bind_vars = [])
        bind_vars.each do |v|
          yield v
        end
      end
      result = []
      exec("insert into test (a, b) values (?, ?)", ["Hello", "Ruby"]) do |v|
        result << v
      end
      puts result.join(" ")
    RUBY
    actual = run_script(script)
    assert_equal("Hello Ruby", actual)
  end

  def test_yield_no_args
    script = <<~'RUBY'
      def m
        yield
      end
      m { puts "called" }
    RUBY
    actual = run_script(script)
    assert_equal("called", actual)
  end

  def test_yield_no_args_with_return_value
    script = <<~'RUBY'
      def m
        result = yield
        puts result
      end
      m { 42 }
    RUBY
    actual = run_script(script)
    assert_equal("42", actual)
  end
end
