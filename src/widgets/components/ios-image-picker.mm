#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import <Foundation/NSString.h>
#include <widgets/components/ios-image-picker.h>

@interface APLViewController : UIViewController <UINavigationControllerDelegate, UIImagePickerControllerDelegate> {}
@end

@implementation APLViewController

- (int)showImagePickerForPhotoPicker {
  [self showImagePickerForSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
  return 5;
}

- (void)showImagePickerForSourceType:(UIImagePickerControllerSourceType)sourceType {
  UIImagePickerController *imagePickerController = [[UIImagePickerController alloc] init];
  imagePickerController.modalPresentationStyle = UIModalPresentationCurrentContext;
  imagePickerController.sourceType = sourceType;
  imagePickerController.delegate = (id) self;

  UIViewController *rootCtrl = [UIApplication sharedApplication].keyWindow.rootViewController;

  [rootCtrl presentViewController:imagePickerController animated:YES completion:nil];
}

- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info {
  UIImage *image = [info valueForKey:UIImagePickerControllerOriginalImage];
  NSData *data = UIImagePNGRepresentation(image);

  const auto &&image2 = QImage::fromData((const unsigned char *) [data bytes], [data length]);

  [picker dismissViewControllerAnimated:YES completion:NULL];

  Q_EMIT ImagePicker::g_currentImagePicker->imageSelected(image2);
  ImagePicker::g_currentImagePicker = NULL;
}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker {
  [picker dismissViewControllerAnimated:YES completion:NULL];
}

@end

ImagePicker *ImagePicker::g_currentImagePicker = NULL;

void ImagePicker::show(void) {
  ImagePicker::g_currentImagePicker = this;
  void *context = [[APLViewController alloc] init];
  [(id) context showImagePickerForPhotoPicker];
}