#include "fmn_hw.h"

const struct fmn_hw_video_type fmn_hw_video_type_nullvideo={
  .name="nullvideo",
  .desc="Dummy video driver that does nothing. eg for automation.",
  .by_request_only=1,
  .objlen=sizeof(struct fmn_hw_video),
};
