class NamespaceTest < Picotest::Test

  if mruby?
    def test_name_space_1
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

end
