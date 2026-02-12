class CaseTest < Picotest::Test
  def test_case_1
    script = <<~RUBY
      dummy = [0,1,2]
      res = case dummy[1]
      when 1
        dummy[0]
        true
      when 2
        false
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_case_2
    script = <<~RUBY
      dummy = [0,1,2]
      res = case dummy[2]
      when 1
        true
      when 2
        dummy[0]
        false
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end

  def test_case_no_else
    script = <<~RUBY
      dummy = [0,1,2]
      res = case dummy[0]
      when 1
        true
      when 2
        false
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_case_else
    script = <<~RUBY
      dummy = [1]
      res = case dummy[0]
      when 2
        false
      else
        String.new
        [{a: 2}, "hello", :ruby][2]
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal(":ruby", actual)
  end

  def test_case_expression_as_an_argument
    script = <<~RUBY
      p case 0
      when 0
        true
      end
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end
end
