Pod::Spec.new do |s|
  s.name        = "TouchVGCore"
  s.version     = "1.0.41"
  s.summary     = "Cross-platform vector drawing library using C++."
  s.homepage    = "https://github.com/touchvg/vgcore"
  s.screenshots = "https://github.com/touchvg/vgcore/raw/master/doc/images/modules.png"
  s.license     = { :type => "LGPL", :file => "LICENSE.md" }
  s.author      = { "Zhang Yungui" => "rhcad@hotmail.com" }
  s.social_media_url    = "http://weibo.com/rhcad"

  s.platform    = :ios, "5.0"
  s.source      = { :git => "https://github.com/touchvg/vgcore.git", :branch => "develop" }
  s.source_files        = "core", "core/**/*.{h,cpp,hpp}"
  s.public_header_files = "core/include/**/*.h"

  s.requires_arc = false
  s.xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'gnu++11',
    'CLANG_CXX_LIBRARY' => 'libc++',
  }
end
