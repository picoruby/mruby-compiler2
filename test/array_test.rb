class ArrayTest < Picotest::Test
  def test_array_basic_case
    script = <<~RUBY
      p([1,2,3,[4,5,6,7,8],9,10])
    RUBY
    actual = run_script(script)
    assert_equal("[1, 2, 3, [4, 5, 6, 7, 8], 9, 10]", actual)
  end

  def test_picoruby_array_split_count_case
    script = <<~RUBY
      ary = %i(
        A00 A01 A02 A03 A04 A05 A06 A07 A08 A09
        A10 A11 A12 A13 A14 A15 A16 A17 A18 A19
        A20 A21 A22 A23 A24 A25 A26 A27 A28 A29
        A30 A31 A32 A33 A34 A35 A36 A37 A38 A39
        A40 A41 A42 A43 A44 A45 A46 A47 A48 A49
        A50 A51 A52 A53 A54 A55 A56 A57 A58 A59
        A60 A61 A62 A63 A64 A65 A66 A67 A68 A69
      )
      p ary[63]
      p ary[64]
      p ary[65]
      p ary[66]
      p ary.size
    RUBY
    actual = run_script(script)
    assert_equal(":A63\n:A64\n:A65\n:A66\n70", actual)
  end

  def test_getter_x_y
    script = <<~RUBY
      ary = [0,1,2,3]
      p ary[2, 3]
    RUBY
    actual = run_script(script)
    assert_equal("[2, 3]", actual)
  end

  def test_setter_x_y
    script = <<~RUBY
      ary = [0,1,2,3]
      ary[4, 10] = 44
      p ary
    RUBY
    actual = run_script(script)
    assert_equal("[0, 1, 2, 3, 44]", actual)
  end
end
