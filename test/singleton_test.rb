class SingletonTest < Picotest::Test

  def test_singleton_klass_2
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      class << String
        def a(n)
          puts "hey" * n
        end
      end
      String.a(3)
    RUBY
    actual = run_script(script)
    assert_equal("heyheyhey", actual)
  end

  def test_singleton_self
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def self.a(v)
        p v
      end
      self.a(:hello)
    RUBY
    actual = run_script(script)
    assert_equal(":hello", actual)
  end

  def test_singleton_lvar
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      lvar = String.new
      def lvar.a(v)
        p v
      end
      lvar.a(:hello_world)
    RUBY
    actual = run_script(script)
    assert_equal(":hello_world", actual)
  end

  def test_singleton_klass_1
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def Array.a(v)
        p v
      end
      Array.a(:hello_hello)
    RUBY
    actual = run_script(script)
    assert_equal(":hello_hello", actual)
  end

end
