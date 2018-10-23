package com.mw.beam.beamwallet.core.views

import android.content.Context
import android.support.v4.content.ContextCompat
import android.support.v4.graphics.drawable.DrawableCompat
import android.util.AttributeSet
import android.view.Gravity
import android.widget.LinearLayout
import com.mw.beam.beamwallet.R
import kotlinx.android.synthetic.main.common_button.view.*

/**
 * Created by vain onnellinen on 10/22/18.
 */
class BeamButton : LinearLayout {
    private var iconResId: Int = Integer.MIN_VALUE
    private var textResId: Int = Integer.MIN_VALUE
    private var textColorResId: Int = Integer.MIN_VALUE
    private var stateListResId: Int = Integer.MIN_VALUE

    constructor(context: Context, attrs: AttributeSet, defStyle: Int) : super(context, attrs, defStyle) {
        init(context, attrs)
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        init(context, attrs)
    }

    constructor(context: Context) : super(context) {
        init(context, null)
    }

    private fun init(context: Context, attrs: AttributeSet?) {
        inflate(context, R.layout.common_button, this)

        this.background = ContextCompat.getDrawable(context, R.drawable.button_common)
        this.orientation = LinearLayout.HORIZONTAL
        this.gravity = Gravity.CENTER

        if (attrs != null) {
            val a = context.theme.obtainStyledAttributes(
                    attrs,
                    R.styleable.BeamButton,
                    0, 0
            )

            iconResId = a.getResourceId(R.styleable.BeamButton_button_icon, Integer.MIN_VALUE)
            textResId = a.getResourceId(R.styleable.BeamButton_button_text, Integer.MIN_VALUE)
            textColorResId = a.getResourceId(R.styleable.BeamButton_button_text_color, Integer.MIN_VALUE)
            stateListResId = a.getResourceId(R.styleable.BeamButton_button_state_list, Integer.MIN_VALUE)

            if (iconResId != Integer.MIN_VALUE) {
                icon.setImageResource(iconResId)
            }

            if (textResId != Integer.MIN_VALUE) {
                text.text = context.getString(textResId)
            }

            if (textColorResId != Integer.MIN_VALUE) {
                text.setTextColor(ContextCompat.getColor(context, textColorResId))
            }

            if (stateListResId != Integer.MIN_VALUE) {
                DrawableCompat.setTintList(this.background, ContextCompat.getColorStateList(context, stateListResId))
            }
        }
    }
}
