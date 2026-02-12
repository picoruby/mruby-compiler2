class IfTest < Picotest::Test
  def test_use_if_value_1
    script = <<~RUBY
      res = if true
        1
      else
        0
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal('1', actual)
  end

  def test_use_if_value_2
    script = <<~RUBY
      res = if false
        1
      else
        0
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal('0', actual)
  end

  def test_use_if_value_3
    script = <<~RUBY
      ary = [0]
      res = if true
        ary[0]
      end
      puts res
    RUBY
    actual = run_script(script)
    assert_equal("0", actual)
  end

  def test_use_else_value_1
    script = <<~RUBY
      ary = [0]
      res = if !true
        ary[0]
      else
        ary[1]
        false
      end
      puts res
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end

  def test_a_complicated_case
    script = <<~RUBY
      proc = Proc.new do puts "hello" end
      ary = [0, true, proc]
      res = if ary[1]
        ary[0] += 1
        ary[2].call
        ary[1]
      end
      puts res
      res = if false
      else
        puts ary[0]
      end
      p res
    RUBY
    actual = run_script(script)
    assert_equal("hello\ntrue\n1\nnil", actual)
  end

  def test_conditional_operator_1
    script = <<~RUBY
      a = [true, false]
      res = !nil ? a[0] : a[1]
      puts res
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_conditional_operator_2
    script = <<~RUBY
      a = [true, false]
      res = !true ? a[0] : a[1]
      puts res
    RUBY
    actual = run_script(script)
    assert_equal("false", actual)
  end
end
