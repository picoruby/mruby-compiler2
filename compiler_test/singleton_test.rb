class SingletonTest < PicoRubyTest

  if @@vm_select == :mruby

    desc "singleton Klass 2"
    assert_equal(<<~RUBY, "heyheyhey")
      class << String
        def a(n)
          puts "hey" * n
        end
      end
      String.a(3)
    RUBY

    pending #"TODO: mruby-3.4.0 has bug"

    desc "singleton self"
    assert_equal(<<~RUBY, ":hello")
      def self.a(v)
        p v
      end
      self.a(:hello)
    RUBY

    desc "singleton lvar"
    assert_equal(<<~RUBY, ":hello_world")
      lvar = String.new
      def lvar.a(v)
        p v
      end
      lvar.a(:hello_world)
    RUBY

    desc "singleton Klass 1"
    assert_equal(<<~RUBY, ":hello_hello")
      def Array.a(v)
        p v
      end
      Array.a(:hello_hello)
    RUBY

  end

end
