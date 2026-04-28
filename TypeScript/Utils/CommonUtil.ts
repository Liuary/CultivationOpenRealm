import * as UE from 'ue';

class CommonUtil {

    static SetImage(Image: UE.Image, Texture: UE.Texture2D): void {
        if (!Image || !Texture) {
            return;
        }

        Image.SetBrushFromTexture(Texture, false);
    }

}

export default CommonUtil;