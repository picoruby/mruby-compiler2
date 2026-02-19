class MethodVisibilityTest < Picotest::Test
  def test_self_call_can_access_private_method
    script = <<~RUBY
      class Foo
        def call_private
          self.secret
        end
        private
        def secret
          :ok
        end
      end
      p Foo.new.call_private
    RUBY
    actual = run_script(script)
    assert_equal(":ok", actual)
  end
end
