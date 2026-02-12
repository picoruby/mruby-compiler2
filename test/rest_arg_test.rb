class RestArgTest < Picotest::Test
  def test_mrubyc_should_handle_restarg
    script = <<~RUBY
      def a(m, *rest)
        p m, rest
      end
      a(1,2,3,4)
    RUBY
    actual = run_script(script)
    assert_equal("1\n[2, 3, 4]", actual)
  end

  def test_restarg_in_block
    script = <<~RUBY
      [0,1].each{|*v| p v}
    RUBY
    actual = run_script(script)
    assert_equal("[0]\n[1]", actual)
  end

  if mruby?
    def test_restarg_in_mruby_mrubyc_doesnt_support_m2_args
      script = <<~RUBY
        def a(m, *rest, m2)
          p m, rest, m2
        end
        a(1,2,3,4)
      RUBY
      actual = run_script(script)
      assert_equal("1\n[2, 3]\n4", actual)
    end

    def test_anonymous_splat_arguments
      script = <<~RUBY
        def my_method(*,**)
          p(*)
          p(**)
        end
        my_method(1, 2, 3, key: :value)
      RUBY
      actual = run_script(script)
      assert_equal("1\n2\n3\n{key: :value}", actual)
    end
  end

end
