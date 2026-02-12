class SingletonTest < Picotest::Test

  if mruby?
    def test_singleton_klass_2
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

end
