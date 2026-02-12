class LambdaTest < Picotest::Test

  if mruby?
    def test_lambda_call
      script = <<~RUBY
        a = -> (m,o=true,*rest,m2) { p m,o,rest,m2 }
        a.call(1,2,3,4)
      RUBY
      actual = run_script(script)
      assert_equal("1\n2\n[3]\n4", actual)
    end

    def test_lambda_shorthand_call
      script = <<~RUBY
        a = -> (m) { p m }
        a.(1)
      RUBY
      actual = run_script(script)
      assert_equal("1", actual)
    end
  end

  end
