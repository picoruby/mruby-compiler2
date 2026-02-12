class SuperTest < Picotest::Test
  def test_super_1
    script = <<~'RUBY'
      class A
        def a(v=0)
          1 + v
        end
      end
      class B < A
        def a
          super
        end
      end
      class C < A
        def a
          super(1)
        end
      end
      p B.new.a
      p C.new.a
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_super_with_block
    script = <<~RUBY
      class A
        def a(v, &b)
          yield(v)
        end
      end
      A.new.a(1){|v| p v}
      class B < A
        def a(v, &b)
          super
        end
      end
      B.new.a(2){|v| p v}
      class C < A
        def a(v, &b)
          super(v) do |v|
            p v + 1
          end
        end
      end
      C.new.a(3)
    RUBY
    actual = run_script(script)
    assert_equal("1\n2\n4", actual)
  end
end
