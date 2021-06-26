#include <window/osxwindow.h>
#include <AppKit/AppKit.h>

void setOSXWindowTitleColor(QMainWindow *w) {
  NSView *view = (NSView *) w->effectiveWinId();
  NSWindow *window = [view window];
  window.titlebarAppearsTransparent = YES;
  window.backgroundColor = [NSColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:1.];
  [window setTitleVisibility:NSWindowTitleHidden];
  // TODO (Fix set title bar height)
  /*NSTitlebarAccessoryViewController *_dummyTitlebarAccessoryViewController = [NSTitlebarAccessoryViewController new];
  NSView *viewF = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];//16
  _dummyTitlebarAccessoryViewController.view = viewF;
  _dummyTitlebarAccessoryViewController.fullScreenMinHeight = 40;
  //[_dummyTitlebarAccessoryViewController setLayoutAttribute:NSLayoutAttributeLeft];
  [window addTitlebarAccessoryViewController:_dummyTitlebarAccessoryViewController];*/
}

QString familyNameFromPostScriptName(QString name) {
  NSString *psName = name.toNSString();
  CTFontDescriptorRef fontDescriptor = (CTFontDescriptorRef) CTFontDescriptorCreateWithNameAndSize(
       (CFStringRef) psName, 12.0);

  CFTypeRef familyName = CTFontDescriptorCopyLocalizedAttribute(fontDescriptor,
                                                                kCTFontFamilyNameAttribute, NULL);
  if (CFGetTypeID(familyName) == CFStringGetTypeID()) {
    QString name = QString::fromCFString((CFStringRef) familyName);
    return name;
  }
  return "nah";
}