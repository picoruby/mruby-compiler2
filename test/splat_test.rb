class SplatTest < Picotest::Test
  if mruby?
    def test_splat_in_fcall_1
      script = <<~RUBY
        p(*1)
      RUBY
      actual = run_script(script)
      assert_equal("1", actual)
    end

    def test_splat_in_fcall_2
      script = <<~RUBY
        p(1, *2)
      RUBY
      actual = run_script(script)
      assert_equal("1\n2", actual)
    end

    def test_splat_in_fcall_3
      script = <<~RUBY
        p(*1,2,p([3,4]))
      RUBY
      actual = run_script(script)
      assert_equal("[3, 4]\n1\n2\n[3, 4]", actual)
    end

    def test_splat_in_fcall_4
      script = <<~RUBY
        p(*1,2,p(*[3,4]))
      RUBY
      actual = run_script(script)
      assert_equal("3\n4\n1\n2\n[3, 4]", actual)
    end

    def test_splat_in_fcall_5
      script = <<~RUBY
        a = *"str"
        p a
      RUBY
      actual = run_script(script)
      assert_equal("[\"str\"]", actual)
    end

    def test_splat_in_fcall_6
      script = <<~RUBY
        p(1,2,*3)
      RUBY
      actual = run_script(script)
      assert_equal("1\n2\n3", actual)
    end

    def test_splat_in_fcall_7
      script = <<~RUBY
        case 0
        when *[0]
          puts "hey"
        end
      RUBY
      actual = run_script(script)
      assert_equal("hey", actual)
    end
  end

end
