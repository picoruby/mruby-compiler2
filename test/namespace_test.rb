class NamespaceTest < Picotest::Test

  def test_name_space_1
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      module A
        module B
          FOO = :foo
        end
      end
      p A::B::FOO
    RUBY
    actual = run_script(script)
    assert_equal(":foo", actual)
  end

  def test_name_space_2
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      module A
      end
      module A::B
        FOO = :foo
      end
      p A::B::FOO
    RUBY
    actual = run_script(script)
    assert_equal(":foo", actual)
  end

  def test_name_space_3
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      module A
        class B
          FOO = :foo
        end
      end
      p A::B::FOO
    RUBY
    actual = run_script(script)
    assert_equal(":foo", actual)
  end

  def test_name_space_fail
    skip "Not supported on mruby/c" unless mruby?
    script = <<~RUBY
      module A
        module B
          FOO = :foo
        end
      end
      begin
        A::FOO
      rescue => e
        puts e.class
      end
    RUBY
    actual = run_script(script)
    assert_equal("NameError", actual)
  end

end
