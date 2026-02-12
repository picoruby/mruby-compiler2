class ClassTest < Picotest::Test
  def test_re_define_array_collect
    script = <<~RUBY
      def collect
        i = 0
        ary = []
        while i < length
          ary[i] = yield self[i]
          i += 1
        end
        return ary
      end
      p [1, 2].collect {|e| e.to_s }
    RUBY
    actual = run_script(script)
    assert_equal("[\"1\", \"2\"]", actual)
  end

  def test_inheritance_from_nested_class
    script = <<~RUBY
      class BLE
        class AttServer
          def get_packet(i)
            puts i * 2
          end
        end
      end
      class MyServer < BLE::AttServer
        def get_packet(i)
          puts i * 3
        end
      end
      BLE::AttServer.new.get_packet(2)
      MyServer.new.get_packet(2)
    RUBY
    actual = run_script(script)
    assert_equal("4\n6", actual)
  end
end
