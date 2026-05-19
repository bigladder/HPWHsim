import numpy as np
import plotly.graph_objects as go
from dash import Dash, Input, Output, ctx, dcc, html, no_update
from plotly.subplots import make_subplots

# ── Fixed parameters ──────────────────────────────────────────────────────────
X2 = np.linspace(0.5, 1.05, 1000)

# ── Defaults ──────────────────────────────────────────────────────────────────
DEFAULT_SETPOINT = 140.0
DEFAULT_TEMP_TURNON_DIFF = 15.0  # °F below setpoint → turn on
DEFAULT_TEMP_SHUTOFF_DIFF = 15.0  # °F below setpoint → shut off
DEFAULT_COMP_TURNON = 125.6  # current turn-on node temperature
DEFAULT_COMP_SHUTOFF = 125.6  # current shut-off node temperature


# ── Figure builder ────────────────────────────────────────────────────────────
def make_figure(temp_turnon_diff, temp_shutoff_diff, setpoint, comp_turnon, comp_shutoff):
    temp_turnon_thresh = setpoint - temp_turnon_diff
    temp_shutoff_thresh = setpoint - temp_shutoff_diff

    x_lo = 32
    x_hi = 212
    X1 = np.linspace(x_lo, x_hi, 500)

    y1_turnon = np.where(X1 < temp_turnon_thresh, 1.0, 0.0)
    y1_shutoff = np.where(X1 < temp_shutoff_thresh, 1.0, 0.0)

    state_turnon = 1.0 if comp_turnon < temp_turnon_thresh else 0.0
    state_shutoff = 1.0 if comp_shutoff < temp_shutoff_thresh else 0.0

    temp_band_lo = min(temp_turnon_thresh, temp_shutoff_thresh)
    temp_band_hi = max(temp_turnon_thresh, temp_shutoff_thresh)

    fig = make_subplots(
        rows=2,
        cols=1,
        subplot_titles=(
            f"Temperature-Based Logic  (turn-on {temp_turnon_thresh:.1f} °F, shut-off {temp_shutoff_thresh:.1f} °F)",
        ),
        vertical_spacing=0.18,
    )

    # ── Subplot 1 ─────────────────────────────────────────────────────────────
    fig.add_trace(
        go.Scatter(
            x=[temp_band_lo, temp_band_hi, temp_band_hi, temp_band_lo, temp_band_lo],
            y=[0, 0, 1, 1, 0],
            fill="toself",
            fillpattern=dict(shape="/", fgcolor="steelblue", bgcolor="rgba(0,0,0,0)", size=8),
            line=dict(color="rgba(0,0,0,0)"),
            name="Deadband",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    fig.add_trace(
        go.Scatter(
            x=X1,
            y=y1_turnon,
            mode="lines",
            line=dict(color="firebrick", width=3, dash="dash"),
            name="Turn-on path (temp ↓)",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    fig.add_trace(
        go.Scatter(
            x=X1,
            y=y1_shutoff,
            mode="lines",
            line=dict(color="darkorange", width=3),
            name="Shut-off path (temp ↑)",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    fig.add_trace(
        go.Scatter(
            x=[setpoint, setpoint],
            y=[0, 0.9],
            mode="lines+markers",
            line=dict(color="black", width=2, dash="dot"),
            marker=dict(
                size=5,
                color="black",
                symbol="diamond",
                line=dict(color="black", width=2),
            ),
            name=f"Setpoint ({setpoint:.0f} °F)",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    # ── Cursor: turn-on node temperature ──────────────────────────────────────
    fig.add_trace(
        go.Scatter(
            x=[comp_turnon],
            y=[state_turnon],
            mode="markers",
            marker=dict(size=14, color="firebrick", symbol="circle", line=dict(color="white", width=2)),
            name="Turn-on node temp",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    # ── Cursor: shut-off node temperature ─────────────────────────────────────
    fig.add_trace(
        go.Scatter(
            x=[comp_shutoff],
            y=[state_shutoff],
            mode="markers",
            marker=dict(size=14, color="darkorange", symbol="circle", line=dict(color="white", width=2)),
            name="Shut-off node temp",
            legend="legend",
        ),
        row=1,
        col=1,
    )

    # ── Reference lines ───────────────────────────────────────────────────────
    fig.update_layout(
        shapes=[
            dict(
                type="line",
                xref="x",
                yref="y",
                x0=temp_turnon_thresh,
                x1=temp_turnon_thresh,
                y0=0,
                y1=1,
                line=dict(color="firebrick", width=1, dash="dash"),
            ),
            dict(
                type="line",
                xref="x",
                yref="y",
                x0=temp_shutoff_thresh,
                x1=temp_shutoff_thresh,
                y0=0,
                y1=1,
                line=dict(color="darkorange", width=1, dash="dash"),
            ),
            dict(
                type="line",
                xref="x",
                yref="y",
                x0=comp_turnon,
                x1=comp_turnon,
                y0=0,
                y1=1,
                line=dict(color="firebrick", width=1.5, dash="dot"),
            ),
            dict(
                type="line",
                xref="x",
                yref="y",
                x0=comp_shutoff,
                x1=comp_shutoff,
                y0=0,
                y1=1,
                line=dict(color="darkorange", width=1.5, dash="dot"),
            ),
        ]
    )

    # ── Annotations ───────────────────────────────────────────────────────────
    def label(xref, yref, x, y, text, **kw):
        return dict(xref=xref, yref=yref, x=x, y=y, text=text, showarrow=False, **kw)

    def arrow(xref, yref, x, y, ax, ay, color):  # noqa: PLR0913
        return dict(
            xref=xref,
            yref=yref,
            axref=xref,
            ayref=yref,
            x=x,
            y=y,
            ax=ax,
            ay=ay,
            text="",
            showarrow=True,
            arrowhead=2,
            arrowsize=1.4,
            arrowwidth=2.5,
            arrowcolor=color,
        )

    annotations = [
        # subplot 1 labels
        label(
            "x",
            "y",
            temp_turnon_thresh,
            1.10,
            f"{temp_turnon_thresh:.1f} °F (turn-on)",
            font=dict(size=10),
            xanchor="center",
        ),
        label(
            "x",
            "y",
            temp_shutoff_thresh,
            1.10,
            f"{temp_shutoff_thresh:.1f} °F (shut-off)",
            font=dict(size=10),
            xanchor="center",
        ),
        label("x", "y", setpoint, 1.10, f"{setpoint:.0f} °F", font=dict(size=10, color="black"), xanchor="center"),
        label("x", "y", (temp_band_lo + temp_band_hi) / 2, 0.50, "Deadband", font=dict(size=12, color="steelblue")),
        # subplot 1 arrows
        arrow("x", "y", x=temp_turnon_thresh - 7, y=0.97, ax=temp_turnon_thresh - 11, ay=0.97, color="firebrick"),
        arrow("x", "y", x=temp_shutoff_thresh + 7, y=0.03, ax=temp_shutoff_thresh + 11, ay=0.03, color="darkorange"),
    ]
    for a in annotations:
        fig.add_annotation(**a)

    fig.update_xaxes(title_text="Tank Node Temperature (°F)", range=[x_lo, x_hi], row=1, col=1)
    fig.update_yaxes(title_text="State", tickvals=[0, 1], ticktext=["OFF", "ON"], range=[-0.1, 1.15], row=1, col=1)
    fig.update_layout(
        title_text="Heat Source Control Band Summary",
        height=820,
        plot_bgcolor="white",
        paper_bgcolor="white",
        legend=dict(x=0.76, y=0.97),
        legend2=dict(x=0.76, y=0.32),
    )
    return fig


# ── Dash app ──────────────────────────────────────────────────────────────────
app = Dash(__name__)

_label_style = {"fontWeight": "bold", "fontFamily": "sans-serif", "flex": "1"}
_input_style = {"width": "65px", "textAlign": "right"}
_panel_style = {
    "width": "260px",
    "padding": "20px",
    "fontFamily": "sans-serif",
    "flexShrink": "0",
    "paddingTop": "10px",
}


def slider_with_input(base_id, label, min_val, max_val, step, value):
    return html.Div(
        [
            html.Div(
                [
                    html.Label(label, style=_label_style),
                    dcc.Input(
                        id=f"{base_id}-input",
                        type="number",
                        value=value,
                        min=min_val,
                        max=max_val,
                        step=step,
                        debounce=True,
                        style=_input_style,
                    ),
                ],
                style={"display": "flex", "alignItems": "center", "gap": "6px"},
            ),
            dcc.Slider(
                id=base_id,
                min=min_val,
                max=max_val,
                step=step,
                value=value,
                tooltip={"placement": "bottom", "always_visible": True},
            ),
        ],
        style={"marginBottom": "24px"},
    )


def _register_sync(base_id):
    slider_id = base_id
    input_id = f"{base_id}-input"

    @app.callback(
        Output(slider_id, "value"),
        Output(input_id, "value"),
        Input(slider_id, "value"),
        Input(input_id, "value"),
        prevent_initial_call=True,
    )
    def _sync(sv, iv):
        return (no_update, sv) if ctx.triggered_id == slider_id else (iv, no_update)


for _id in ["setpoint", "temp-turnon-diff", "temp-shutoff-diff", "comp-turnon", "comp-shutoff"]:
    _register_sync(_id)

app.layout = html.Div(
    [
        html.H3("Heat Source Control Band", style={"fontFamily": "sans-serif"}),
        # ── Controls row ──────────────────────────────────────────────────────────
        html.Div(
            [
                # Left: setpoint + differential controls
                html.Div(
                    [
                        slider_with_input(
                            "setpoint", "Setpoint (°F)", min_val=104, max_val=176, step=1.0, value=DEFAULT_SETPOINT
                        ),
                        slider_with_input(
                            "temp-turnon-diff",
                            "Turn-on differential (°F below setpoint)",
                            min_val=0,
                            max_val=36,
                            step=0.2,
                            value=DEFAULT_TEMP_TURNON_DIFF,
                        ),
                        slider_with_input(
                            "temp-shutoff-diff",
                            "Shut-off differential (°F below setpoint)",
                            min_val=0,
                            max_val=36,
                            step=0.2,
                            value=DEFAULT_TEMP_SHUTOFF_DIFF,
                        ),
                    ],
                    style=_panel_style,
                ),
                # Right: cursor / node temperature controls
                html.Div(
                    [
                        slider_with_input(
                            "comp-turnon",
                            "Turn-on comparison node current state (°F)",
                            min_val=32,
                            max_val=212,
                            step=0.2,
                            value=DEFAULT_COMP_TURNON,
                        ),
                        slider_with_input(
                            "comp-shutoff",
                            "Shut-off comparison node current state (°F)",
                            min_val=32,
                            max_val=212,
                            step=0.2,
                            value=DEFAULT_COMP_SHUTOFF,
                        ),
                    ],
                    style=_panel_style,
                ),
            ],
            style={"display": "flex", "alignItems": "flex-start"},
        ),
        # ── Graph ─────────────────────────────────────────────────────────────────
        dcc.Graph(id="band-graph"),
    ],
    style={"fontFamily": "sans-serif"},
)


@app.callback(
    Output("band-graph", "figure"),
    Input("temp-turnon-diff", "value"),
    Input("temp-shutoff-diff", "value"),
    Input("setpoint", "value"),
    Input("comp-turnon", "value"),
    Input("comp-shutoff", "value"),
)
def update(temp_turnon_diff, temp_shutoff_diff, setpoint, comp_turnon, comp_shutoff):
    return make_figure(temp_turnon_diff, temp_shutoff_diff, setpoint, comp_turnon, comp_shutoff)


def main():
    app.run(debug=True)


if __name__ == "__main__":
    main()
