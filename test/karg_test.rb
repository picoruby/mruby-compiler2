class KargTest < Picotest::Test
  def test_def_method_with_karg
    script = <<~RUBY
      def m1(k: 1)
        puts k
      end
      m1
      m1(k:2)
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_required_keyword
    script = <<~RUBY
      def m2(k:)
        puts k
      end
      m2(k: true)
    RUBY
    actual = run_script(script)
    assert_equal("true", actual)
  end

  def test_complicated_case
    script = <<~RUBY
      def m3(a, b, o1 = 1, o2 = 2, *c, k1:, k2: 3, k3:)
        puts k1 + k2 + k3
      end
      m3("dummy", "dummy", k3: 1, k2: 2, k1: 10)
    RUBY
    actual = run_script(script)
    assert_equal("13", actual)
  end

  def test_block_argument
    script = <<~RUBY
      p = Proc.new do |a:, b: 11|
        a + b
      end
      puts p.call(b: 12, a: 13)
    RUBY
    actual = run_script(script)
    assert_equal("25", actual)
  end

  def test_block_argument_2
    script = <<~RUBY
      def task(opt: {})
        yield(opt)
      end
      task(opt: {a: 0}) do |opt|
        p opt
      end
    RUBY
    actual = run_script(script)
    assert_equal("{a: 0}", actual)
  end

  def test_karg_and_dict
    script = <<~RUBY
      def combined_with_dict(k1: 0, k2:, **dict)
        k1 + k2 + dict[:k3] + dict[:k4]
      end
      p combined_with_dict(k2: 2, k1: 1, k3: 3, k4: 4)
    RUBY
    actual = run_script(script)
    assert_equal("10", actual)
  end
end
