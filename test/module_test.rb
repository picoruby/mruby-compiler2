class ModuleTest < Picotest::Test

  if mruby?
    def test_include_a_module
      script = <<~RUBY
        module A
          def a
            p 0
          end
        end
        include A
        self.a
      RUBY
      actual = run_script(script)
      assert_equal("0", actual)
    end
  end

end
