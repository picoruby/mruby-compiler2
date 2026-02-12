class DefTest < Picotest::Test
  def test_def_method_without_arg
    script = <<~RUBY
      def my_method
        "My method!"
      end
      puts my_method
    RUBY
    actual = run_script(script)
    assert_equal("My method!", actual)
  end

  def test_def_method_with_a_mandatory_arg
    script = <<~RUBY
      def my_method(arg)
        arg
      end
      puts my_method("My arg")
    RUBY
    actual = run_script(script)
    assert_equal("My arg", actual)
  end

  def test_endless_def
    script = <<~RUBY
      def my_method(arg) = arg.to_i
      puts my_method(nil)
    RUBY
    actual = run_script(script)
    assert_equal("0", actual)
  end

  def test_def_method_with_an_optional_arg
    script = <<~RUBY
      def my_method(arg = "default")
        arg
      end
      puts my_method
    RUBY
    actual = run_script(script)
    assert_equal("default", actual)
  end

  def test_def_method_with_three_optional_args
    script = <<~RUBY
      def my_method(arg1 = "default", arg2=nil, arg3=0)
        p arg1, arg2, arg3
      end
      my_method
    RUBY
    actual = run_script(script)
    assert_equal("\"default\"\nnil\n0", actual)
  end

  def test_def_method_with_a_mandatory_arg_and_an_optional_arg
    script = <<~RUBY
      def my_method(marg, opt = "optional")
        puts marg, opt
      end
      my_method("hey", "you")
    RUBY
    actual = run_script(script)
    assert_equal("hey\nyou", actual)
  end

  def test_block
    script = <<~RUBY
      def m(&block)
        block.call
      end
      m { puts 1 }
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_marg_optarg_and_block
    script = <<~RUBY
      def m(a,b,&block)
        block.call
      end
      m(8,9) { puts 1 }
    RUBY
    actual = run_script(script)
    assert_equal("1", actual)
  end

  def test_marg_optarg_and_block_part_2
    script = <<~RUBY
      def m(a,b,&block)
        block.call(a,b)
      end
      m(8,9) { |x, y| puts x, y }
    RUBY
    actual = run_script(script)
    assert_equal("8\n9", actual)
  end

  def test_def_exclamation
    script = <<~RUBY
      p(def !() end)
    RUBY
    actual = run_script(script)
    assert_equal(":!", actual)
  end

  def test_def_backtick
    script = <<~RUBY
      p(def `() end)
    RUBY
    actual = run_script(script)
    assert_equal(":`", actual)
  end

  def test_def_equal_equal
    script = <<~RUBY
      p(def ==() end)
    RUBY
    actual = run_script(script)
    assert_equal(":==", actual)
  end

  def test_def_spaceship
    script = <<~RUBY
      p(def <=>() end)
    RUBY
    actual = run_script(script)
    assert_equal(":<=>", actual)
  end

  def test_def_left_shift
    script = <<~RUBY
      p(def <<() end)
    RUBY
    actual = run_script(script)
    assert_equal(":<<", actual)
  end

  def test_def_right_shift
    script = <<~RUBY
      p(def >>() end)
    RUBY
    actual = run_script(script)
    assert_equal(":>>", actual)
  end

  def test_def_less_than
    script = <<~RUBY
      p(def <() end)
    RUBY
    actual = run_script(script)
    assert_equal(":<", actual)
  end

  def test_def_greater_than
    script = <<~RUBY
      p(def >() end)
    RUBY
    actual = run_script(script)
    assert_equal(":>", actual)
  end

  def test_multi_target_case
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      def m((a, b), *c)
        p a, b, c
      end
      m(1,2,3)
    RUBY
    actual = run_script(script)
    assert_equal("1\nnil\n[2, 3]", actual)
  end
end
