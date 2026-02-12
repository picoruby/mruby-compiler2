class MasgnTest < Picotest::Test
  def test_basic_case_1
    script = <<~RUBY
      a, b = 1, 2
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_basic_case_2
    script = <<~RUBY
      a, b = [1, 2]
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_rhs_has_only_one_item
    script = <<~RUBY
      a, b = 1
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("1\nnil", actual)
  end

  def test_baz_should_be_nil
    script = <<~RUBY
      baz = true
      foo, bar, baz = 1, 2
      p foo, bar, baz
    RUBY
    actual = run_script(script)
    assert_equal("1\n2\nnil", actual)
  end

  def test_3_should_be_ignored
    script = <<~RUBY
      foo, bar = 1, 2, 3
      p foo, bar
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_single_pre
    script = <<~RUBY
      foo      = 1, 2, 3
      p foo
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_single_rest
    script = <<~RUBY
      *foo     = 1, 2, 3
      p foo
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3]", actual)
  end

  def test_rest_gets_the_rest
    script = <<~RUBY
      foo,*bar = 1, 2, 3
      p foo, bar
    RUBY
    actual = run_script(script)
    assert_equal("1\n[2, 3]", actual)
  end

  def test_post_should_be_nil
    script = <<~RUBY
      post = true
      pre1, pre2, *rest, post = 1, 2
      p post
    RUBY
    actual = run_script(script)
    assert_equal("nil", actual)
  end

  def test_grouping
    script = <<~RUBY
      (foo, bar), baz = [1, 2], 3
      p foo, bar, baz
    RUBY
    actual = run_script(script)
    assert_equal("1\n2\n3", actual)
  end

  def test_complicated_case
    script = <<~'RUBY'
      class C
        attr_accessor :foo, :bar
        def foo=( v )
          @foo = v
        end
        def []=(i,v)
          @bar = ["a", "b", "c"]
          @bar[i] = v
        end
      end
      obj = C.new
      obj.foo, obj[2] = 1, 2, 3, 4
      p obj.foo, obj.bar
    RUBY
    actual = run_script(script)
    assert_equal("1\n[\"a\", \"b\", 2]", actual)
  end

  def test_trailing_comma_in_lhs_basic
    script = <<~RUBY
      a, b, = []
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("nil\nnil", actual)
  end

  def test_trailing_comma_in_lhs_with_values
    script = <<~RUBY
      a, b, = [1, 2]
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  def test_trailing_comma_in_lhs_excess_values
    script = <<~RUBY
      a, b, = [1, 2, 3]
      p a, b
    RUBY
    actual = run_script(script)
    assert_equal("1\n2", actual)
  end

  if mruby?
    def test_rhs_has_only_one_item_lhs_has_a_rest_and_a_post
      script = <<~RUBY
        a, b, *c, d = 1
        p a, b, c, d
      RUBY
      actual = run_script(script)
      assert_equal("1\nnil\n[]\nnil", actual)
    end

    def test_rhs_has_only_one_item_which_is_an_array_lhs_has_a_rest_and_a_post
      script = <<~RUBY
        ary = [0, 1]
        a, *b, c = ary
        p a, b, c
      RUBY
      actual = run_script(script)
      assert_equal("0\n[]\n1", actual)
    end

    def test_splat_in_rhs
      script = <<~RUBY
        ary = [9, 8]
        a,*b,c,d=1,*ary,3,4
        p a,b,c,d
      RUBY
      actual = run_script(script)
      assert_equal("1\n[9, 8]\n3\n4", actual)
    end
  end

end
