MRuby::Gem::Specification.new('mruby-compiler2') do |spec|
  spec.license = 'MIT'
  spec.author  = 'HASUMI Hitoshi'
  spec.summary = 'mruby compiler using a universal parser'

  lib_dir = "#{dir}/lib"
  prism_dir = "#{lib_dir}/prism"
  static_prism = "#{prism_dir}/build/libprism.a"
  cc.include_paths << "#{dir}/include"
  cc.include_paths << "#{prism_dir}/include"

  directory prism_dir do
    FileUtils.cd lib_dir do
      sh "git clone https://github.com/ruby/prism.git"
    end
    FileUtils.cd prism_dir do
      sh "git checkout 532e4cc209818f52075bd4b2dc2f353d7da1bfbe"
    end
  end

  task :prism_templates => prism_dir do
    FileUtils.cd prism_dir do
      sh "templates/template.rb"
    end
  end

  Rake::Task[:prism_templates].invoke

  Dir.glob(["#{prism_dir}/src/**/*.c", "#{dir}/src/.c"]).map do |src|
    obj = objfile(src.pathmap("#{build_dir}/lib/%n"))
    build.libmruby_objs << obj
    task obj => [src] do |f|
      cc.run f.name, f.prerequisites.first
    end
  end

  build.libmruby_objs << objs

  task :deep_clean do
    rm_rf prism_dir
  end

end

